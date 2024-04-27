#include "OperationBuilder.h"
#include "Streams/CallbackWritableStream.h"
#include <functional>

OperationBuilder::OperationBuilder(const Core * core, uint32_t siteId, Tag tag)
  : siteId(siteId), tag(tag), core(core), clock(&core->clock) {}

IReadableStream<RefCounted<const LogOperation>> & OperationBuilder::getReadableStream()
{
  return readableStream;
}

template <typename T>
T * OperationBuilder::appendData()
{
  return AppendData<T>(getOpBuffer());
}

template <typename T>
T * OperationBuilder::appendData(size_t size)
{
  return AppendData<T>(getOpBuffer(), size);
}

template <typename T>
T * OperationBuilder::AppendData(std::basic_string<char> & buffer)
{
  return AppendData<T>(buffer, sizeof(T));
}

template <typename T>
T * OperationBuilder::AppendData(std::basic_string<char> & buffer, size_t size)
{
  size_t prevSize = buffer.size();
  buffer.append(size, 0);
  return reinterpret_cast<T *>(buffer.data() + prevSize);
}

std::basic_string<char> & OperationBuilder::getOpBuffer()
{
  if (groupContexts.size() == 0)
  {
    startLogOperation();
  }

  return opBuffer;
}

Timestamp OperationBuilder::finishOp()
{
  if (groupContexts.size() == 0)
  {
    return commitLogOperation();
  }

  groupContexts.back().opCount++;

  return getCurrentTimestamp();
}

void OperationBuilder::startLogOperation()
{
  if (opBuffer.size() > 0)
  {
    //an operation already exists
    return;
  }

  //using the static AppendData here to avoid calling getOpBuffer infinitely
  auto op = AppendData<LogOperation>(opBuffer, LogOperation::getSizeWithoutOp());
  op->tag = tag;
  op->ts = getNextTimestamp();
}

Timestamp OperationBuilder::commitLogOperation()
{
  LogOperation * op = reinterpret_cast<LogOperation *>(opBuffer.data());
  Timestamp result = op->ts;

  if (enabled) //NOTE: this might change later (lazy support for view only)
  {
    auto opCopy = new uint8_t[opBuffer.size()];
    memcpy(opCopy, opBuffer.data(), opBuffer.size());
    RefCounted<const LogOperation> rc(reinterpret_cast<const LogOperation *>(opCopy));

    readableStream.writeToDestination(rc);
  }

  opBuffer.clear();

  return result;
}

void OperationBuilder::startGroup(const OperationType & groupType)
{
  if (groupType != OperationType::AtomicGroupOperation && groupContexts.size() > 0)
  {
    //protect against more than one group context (not currently allowed)
    //for now, just clear the previous (unfinished) group, presumably there is a bug
    discardGroup();
  }

  //NOTE: this can change the buffer (e.g. appends an op header)
  //  consider this if returning early
  auto & buffer = getOpBuffer();

  GroupContext ctx;
  ctx.type = groupType;
  ctx.bufferOffset = buffer.size();
  ctx.opCount = 0;
  groupContexts.push_back(ctx);

  if (groupType == OperationType::AtomicGroupOperation)
  {
    auto op = appendData<AtomicGroupOperation>();
    op->type = OperationType::AtomicGroupOperation;
  }
  else
  {
    auto op = appendData<GroupOperation>();
    op->type = OperationType::GroupOperation;
  }
}

Timestamp OperationBuilder::commitGroup()
{
  if (groupContexts.size() == 0)
  {
    //no group to commit
    return Timestamp::Null;
  }

  GroupContext & ctx = groupContexts.back();

  if (ctx.type == OperationType::AtomicGroupOperation)
  {
    auto op = reinterpret_cast<AtomicGroupOperation *>(opBuffer.data() + ctx.bufferOffset);
    op->length = opBuffer.size() - ctx.bufferOffset - sizeof(AtomicGroupOperation);

    if (op->length == 0)
    {
      groupContexts.pop_back();
      opBuffer.clear();
      return Timestamp::Null;
    }
  }
  else
  {
    auto op = reinterpret_cast<GroupOperation *>(opBuffer.data() + ctx.bufferOffset);
    op->length = opBuffer.size() - ctx.bufferOffset - sizeof(GroupOperation);

    if (op->length == 0)
    {
      groupContexts.pop_back();
      opBuffer.clear();
      return Timestamp::Null;
    }
  }

  size_t opCountChange;
  if (groupContexts.back().type == OperationType::AtomicGroupOperation)
  {
    opCountChange = 1;
  }
  else
  {
    opCountChange = groupContexts.back().opCount;
  }

  groupContexts.pop_back();
  if (groupContexts.size() > 0)
  {
    //increase the overall op count for the previous group
    groupContexts.back().opCount += opCountChange;

    //decrement the op count so the effective op count change is 0
    groupContexts.back().opCount--;
  }
  return finishOp();
}

void OperationBuilder::discardGroup()
{
  if (groupContexts.size() == 0)
  {
    //no group to discard
    return;
  }

  // groupContexts.pop_back();
  groupContexts.clear();
  opBuffer.clear();
}

// Timestamp OperationBuilder::applyOperations(const Operation * data, size_t length)
// {
//   OperationIterator it(data, length);

//   if (opBuffer.size() > 0)
//   {
//     opBuffer.reserve(opBuffer.size() + length);
//   }

//   //use the current timestamp instead of next ts and an offset starting at 1
//   //this is because right now relative timestamps in op lists assume that
//   //  the first timestamp is 1,0
//   Timestamp startTs = getCurrentTimestamp();
//   uint32_t offset = 1;

//   while (*it != nullptr)
//   {
//     const Operation * op = *it;

//     const_cast<Operation *>(op)->transformTimestamps(startTs);

//     applyOperation(op, op->getSize());

//     ++it;
//     ++offset;
//   }

//   return startTs + 1;
// }

Timestamp OperationBuilder::applyOperation(const Operation * op, size_t length)
{
  OperationIterator it(op, length);
  if (*it == nullptr)
  {
    return Timestamp::Null;
  }

  auto & buffer = getOpBuffer();
  buffer.append(reinterpret_cast<const char *>(op), length);
  return finishOp();
}

Timestamp OperationBuilder::applyOperation(const LogOperation & op)
{
  auto & buffer = getOpBuffer();
  buffer.append(reinterpret_cast<const char *>(&op.op), op.op.getSize());
  return finishOp();
}

Timestamp OperationBuilder::applyOperation(const Operation & op)
{
  auto & buffer = getOpBuffer();
  buffer.append(reinterpret_cast<const char *>(&op), op.getSize());
  return finishOp();
}

IWritableStream<RefCounted<const LogOperation>> * OperationBuilder::createApplyStream()
{
  //use current ts instead of next ts because ops use (1, 0) to refer to the
  //  first operation in a type log; this makes adding the offset work properly
  Timestamp offset = this->getCurrentTimestamp();

  auto * callbackStream = new CallbackWritableStream<RefCounted<const LogOperation>>(
    [&, offset](const RefCounted<const LogOperation> & op)
    {
      auto innerOp = const_cast<Operation *>(&op->op);
      if (op->ts.isNull())
      {
        //NOTE: mutate the op; this is needed to apply types directly
        //  this is questionable but currently doesn't pose any problems
        innerOp->transformTimestamps(offset);
      }

      applyOperation(*innerOp);
    },
    []()
    {

    });

  return callbackStream;
}


Timestamp OperationBuilder::undoOperation(const LogOperation & op)
{
  UndoGroup(getOpBuffer(), op);
  return finishOp();
}

IWritableStream<RefCounted<const LogOperation>> * OperationBuilder::createUndoStream()
{
  auto * callbackStream = new CallbackWritableStream<RefCounted<const LogOperation>>(
    [&](const RefCounted<const LogOperation> & op)
    {
      undoOperation(*op);
    },
    []()
    {

    });

  return callbackStream;
}

Timestamp OperationBuilder::setAttribute(uint8_t attributeId, const std::string & data)
{
  return setAttribute(attributeId, reinterpret_cast<const uint8_t *>(data.data()), data.size());
}

Timestamp OperationBuilder::setAttribute(uint8_t attributeId, const uint8_t * data, uint32_t length)
{
  if (groupContexts.size() == 0 ||
    groupContexts.back().type != OperationType::AtomicGroupOperation)
  {
    //not in an atomic group
    return Timestamp::Null;
  }

  AttributeSet(getOpBuffer(), attributeId, data, length);
  //decrement the op count so the effective op count change is 0
  groupContexts.back().opCount--;
  return finishOp();
}

NodeId OperationBuilder::createNode(const NodeType & nodeType)
{
  return createNode(nodeType.toString());
}

NodeId OperationBuilder::createNode(const std::string & nodeType)
{
  NodeCreate(getOpBuffer(), reinterpret_cast<const uint8_t *>(nodeType.data()), nodeType.size());
  return {finishOp(), 0};
}

NodeId OperationBuilder::createContainerNode(const NodeType & nodeType, const NodeType & childType)
{
  return createContainerNode(nodeType.toString(), childType.toString());
}

NodeId OperationBuilder::createContainerNode(const std::string & nodeType, const std::string & childType)
{
  startGroup(OperationType::AtomicGroupOperation);
  createNode(nodeType);
  setAttribute(0, childType);
  return {commitGroup(), 0};
}

EdgeId OperationBuilder::addChild(const NodeId & parentId, const NodeId & childId)
{
  return addChild(parentId, childId, nullptr, 0);
}

EdgeId OperationBuilder::addChild(const NodeId & parentId, const NodeId & childId, const std::string edgeData)
{
  return addChild(parentId, childId,
    reinterpret_cast<const uint8_t *>(edgeData.data()), edgeData.size());
}

EdgeId OperationBuilder::addChild(const NodeId & parentId, const NodeId & childId, const uint8_t * data, uint32_t length)
{
  if (length == 0)
  {
    EdgeCreate(getOpBuffer(), parentId, childId);
    return {finishOp(), 0};
  }

  startGroup(OperationType::AtomicGroupOperation);
  EdgeCreate(getOpBuffer(), parentId, childId);
  finishOp();
  setAttribute(ContainerNode::AttributeType::ChildType, data, length);
  return {commitGroup(), 0};
}

Timestamp OperationBuilder::removeChild(const NodeId & parentId, const EdgeId & edgeId)
{
  EdgeDelete(getOpBuffer(), parentId, edgeId);
  return finishOp();
}

std::string OperationBuilder::createPositionBetweenEdges(const EdgeId & firstEdgeId, const EdgeId & secondEdgeId) const
{
  Position newPosition(firstEdgeId);
  return CreatePosition(newPosition);
}

std::string OperationBuilder::createPositionFromIndex(const NodeId & parentId, size_t index) const
{
  const Node * parent = core->getExistingNode(parentId);
  if (parent == nullptr)
  {
    return std::string();
  }

  NodeType parentType = parent->getBaseType();
  Position newPosition;

  if (parentType == PrimitiveNodeTypes::List())
  {
    const ListNode * listNode = static_cast<const ListNode *>(parent);

    size_t i = 0;
    ListEdge * prevEdge = nullptr;
    ListEdge * currentEdge = listNode->children;
    while (currentEdge != nullptr)
    {
      if (i == index)
      {
        break;
      }

      if (currentEdge->childId.isNull() == false)
      {
        i++;
      }

      prevEdge = currentEdge;
      currentEdge = currentEdge->nextChild;
    }

    if (prevEdge != nullptr)
    {
      newPosition.prevEdgeId = prevEdge->edgeId;
    }
  }
  else
  {
    //not an ordered container type
    return std::string();
  }

  return CreatePosition(newPosition);
}

std::string OperationBuilder::createPositionFromEdge(const NodeId & parentId, const EdgeId & sourceEdgeId) const
{
  const Node * parent = core->getExistingNode(parentId);
  if (parent == nullptr)
  {
    return std::string();
  }

  NodeType parentType = parent->getBaseType();
  Position newPosition;

  if (parentType == PrimitiveNodeTypes::List())
  {
    const ListNode * listNode = static_cast<const ListNode *>(parent);

    ListEdge * prevEdge = nullptr;
    ListEdge * currentEdge = listNode->children;
    while (currentEdge != nullptr)
    {
      if (currentEdge->edgeId == sourceEdgeId)
      {
        break;
      }

      prevEdge = currentEdge;
      currentEdge = currentEdge->nextChild;
    }

    if (prevEdge != nullptr)
    {
      newPosition.prevEdgeId = prevEdge->edgeId;
    }
  }
  else
  {
    //not an ordered container type
    return std::string();
  }

  return CreatePosition(newPosition);
}

std::string OperationBuilder::createPositionAbsolute(double position) const
{
  return CreatePositionAbsolute(position);
}

std::string OperationBuilder::CreatePositionAbsolute(double position)
{
  std::string out;
  out.append(sizeof(double), 0);
  *(reinterpret_cast<double *>(out.data())) = position;
  return out;
}

std::string OperationBuilder::CreatePosition(const Position & position)
{
  std::string out;
  out.append(sizeof(EdgeId), 0);
  *(reinterpret_cast<EdgeId *>(out.data())) = position.prevEdgeId;
  return out;
}

Timestamp OperationBuilder::setValueAuto(const NodeId & nodeId, double value)
{
  const Node * node = core->getExistingNode(nodeId);
  if (node == nullptr)
  {
    return Timestamp::Null;
  }

  NodeType baseType = node->getBaseType();
  auto data = DoubleToValueData(baseType, value);
  if (data.size() == 0)
  {
    return Timestamp::Null;
  }

  return setValue(nodeId, reinterpret_cast<uint8_t *>(data.data()), data.size());
}

Timestamp OperationBuilder::setValue(const NodeId & nodeId, const uint8_t * data, uint32_t length)
{
  ValueSet(getOpBuffer(), nodeId, data, length);
  return finishOp();
}

Timestamp OperationBuilder::setValuePreviewAuto(const NodeId & nodeId, double value)
{
  const Node * node = core->getExistingNode(nodeId);
  if (node == nullptr)
  {
    return Timestamp::Null;
  }

  NodeType baseType = node->getBaseType();
  auto data = DoubleToValueData(baseType, value);
  if (data.size() == 0)
  {
    return Timestamp::Null;
  }

  return setValuePreview(nodeId, reinterpret_cast<uint8_t *>(data.data()), data.size());
}

Timestamp OperationBuilder::setValuePreview(const NodeId & nodeId, const uint8_t * data, uint32_t length)
{
  ValueSetPreview(getOpBuffer(), nodeId, data, length);
  return finishOp();
}

Timestamp OperationBuilder::clearValuePreview(const NodeId & nodeId)
{
  return setValuePreview(nodeId, nullptr, 0);
}

void OperationBuilder::insertText(const NodeId & nodeId, size_t offset, const std::string & str)
{
  insertText(nodeId, offset, reinterpret_cast<const uint8_t *>(str.c_str()), str.length());
}

void OperationBuilder::insertText(const NodeId & nodeId, size_t offset, const uint8_t * data, size_t length)
{
  const Node * node = core->getExistingNode(nodeId);
  if (node == nullptr ||
    PrimitiveNodeTypes::isBlockValueNodeType(node->getBaseType()) == false)
  {
    return;
  }

  auto blockValueNode = static_cast<const BlockValueNode<char> *>(node);
  blockValueNode->value.computeInsertions(offset, data, length,
    [this, &nodeId](const Timestamp & blockId, uint32_t offset, const uint8_t * data, uint32_t length)
    {
      BlockValueInsertAfter(getOpBuffer(), nodeId, blockId, offset, data, length);
      finishOp();
    });
}

void OperationBuilder::insertTextAtFront(const NodeId & nodeId, const std::string & str)
{
  insertTextAtFront(nodeId, reinterpret_cast<const uint8_t *>(str.c_str()), str.length());
}

void OperationBuilder::insertTextAtFront(const NodeId & nodeId, const uint8_t * data, size_t length)
{
  BlockValueInsertAfter(getOpBuffer(), nodeId, Timestamp::Null, 0, data, length);
  finishOp();
}

void OperationBuilder::deleteText(const NodeId & nodeId, size_t offset, size_t length)
{
  const Node * node = core->getExistingNode(nodeId);
  if (node == nullptr ||
    PrimitiveNodeTypes::isBlockValueNodeType(node->getBaseType()) == false)
  {
    return;
  }

  auto blockValueNode = static_cast<const BlockValueNode<char> *>(node);
  blockValueNode->value.computeDeletions(offset, length,
    [this, &nodeId](const Timestamp & blockId, uint32_t offset, uint32_t length)
    {
      BlockValueDeleteAfter(getOpBuffer(), nodeId, blockId, offset, length);
      finishOp();
    });
}

void OperationBuilder::deleteInheritedText(const NodeId & nodeId)
{
  BlockValueDeleteAfter(getOpBuffer(), nodeId, nodeId.ts, 0,
    BlockData<char>::maxLength);
  finishOp();
}

void OperationBuilder::UndoGroup(std::basic_string<char> & buffer, const LogOperation & op)
{
  size_t undoOpOffset = buffer.size();
  auto undoOp = AppendData<UndoGroupOperation>(buffer);

  if (op.op.type == OperationType::UndoGroupOperation ||
    op.op.type == OperationType::RedoGroupOperation)
  {
    auto prevUndoOp = reinterpret_cast<const UndoGroupOperation *>(&op.op);

    *undoOp = *prevUndoOp;
    undoOp->type = (op.op.type == OperationType::UndoGroupOperation)
      ? OperationType::RedoGroupOperation : OperationType::UndoGroupOperation;
    buffer.append(reinterpret_cast<const char *>(&prevUndoOp->data), prevUndoOp->length);
  }
  else
  {
    undoOp->type = OperationType::UndoGroupOperation;
    undoOp->prevTs = op.ts;

    size_t prevSize = buffer.size();

    if (op.op.type == OperationType::GroupOperation)
    {
      const GroupOperation * groupOp = static_cast<const GroupOperation *>(&op.op);

      OperationIterator it(reinterpret_cast<const Operation *>(&groupOp->data),
        groupOp->length);
      while (*it != nullptr)
      {
        Undo(buffer, *it);
        ++it;
      }
    }
    else
    {
      Undo(buffer, &op.op);
    }

    undoOp = reinterpret_cast<UndoGroupOperation *>(buffer.data() + undoOpOffset);
    undoOp->length = buffer.size() - prevSize;
  }
}

void OperationBuilder::Undo(std::basic_string<char> & buffer, const Operation * op)
{
  switch (op->type)
  {
    case OperationType::AtomicGroupOperation:
    {
      const AtomicGroupOperation * groupOp = static_cast<const AtomicGroupOperation *>(op);
      OperationIterator it(reinterpret_cast<const Operation *>(&groupOp->data), groupOp->length);

      if (*it != nullptr)
      {
        Undo(buffer, *it);
      }
      break;
    }

    case OperationType::EdgeCreateOperation:
    {
      UndoEdgeCreateOperation newOp(static_cast<const EdgeCreateOperation *>(op));
      buffer.append(reinterpret_cast<const char *>(&newOp), newOp.getSize());
      break;
    }
    case OperationType::UndoEdgeCreateOperation:
    {
      UndoEdgeCreateOperation newOp(static_cast<const UndoEdgeCreateOperation *>(op));
      buffer.append(reinterpret_cast<const char *>(&newOp), newOp.getSize());
      break;
    }

    case OperationType::EdgeDeleteOperation:
    {
      UndoEdgeDeleteOperation newOp(static_cast<const EdgeDeleteOperation *>(op));
      buffer.append(reinterpret_cast<const char *>(&newOp), newOp.getSize());
      break;
    }
    case OperationType::UndoEdgeDeleteOperation:
    {
      UndoEdgeDeleteOperation newOp(static_cast<const UndoEdgeDeleteOperation *>(op));
      buffer.append(reinterpret_cast<const char *>(&newOp), newOp.getSize());
      break;
    }

    case OperationType::ValueSetOperation:
    {
      UndoValueSetOperation newOp(static_cast<const ValueSetOperation *>(op));
      buffer.append(reinterpret_cast<const char *>(&newOp), newOp.getSize());
      break;
    }
    case OperationType::UndoValueSetOperation:
    {
      UndoValueSetOperation newOp(static_cast<const UndoValueSetOperation *>(op));
      buffer.append(reinterpret_cast<const char *>(&newOp), newOp.getSize());
      break;
    }

    case OperationType::BlockValueInsertAfterOperation:
    {
      UndoBlockValueInsertAfterOperation newOp(static_cast<const BlockValueInsertAfterOperation *>(op));
      buffer.append(reinterpret_cast<const char *>(&newOp), newOp.getSize());
      break;
    }
    case OperationType::UndoBlockValueInsertAfterOperation:
    {
      UndoBlockValueInsertAfterOperation newOp(static_cast<const UndoBlockValueInsertAfterOperation *>(op));
      buffer.append(reinterpret_cast<const char *>(&newOp), newOp.getSize());
      break;
    }

    case OperationType::BlockValueDeleteAfterOperation:
    {
      UndoBlockValueDeleteAfterOperation newOp(static_cast<const BlockValueDeleteAfterOperation *>(op));
      buffer.append(reinterpret_cast<const char *>(&newOp), newOp.getSize());
      break;
    }
    case OperationType::UndoBlockValueDeleteAfterOperation:
    {
      UndoBlockValueDeleteAfterOperation newOp(static_cast<const UndoBlockValueDeleteAfterOperation *>(op));
      buffer.append(reinterpret_cast<const char *>(&newOp), newOp.getSize());
      break;
    }

    default:
      buffer.append(1, static_cast<char>(OperationType::NoOpOperation));
  }
}

void OperationBuilder::AttributeSet(std::basic_string<char> & buffer, const uint8_t attributeId, const uint8_t * data, uint16_t length)
{
  auto op = AppendData<SetAttributeOperation>(buffer,
    sizeof(SetAttributeOperation) + length);
  op->type = OperationType::SetAttributeOperation;
  op->attributeId = attributeId;
  op->length = length;
  std::memcpy(&op->data, data, length);
}

void OperationBuilder::NodeCreate(std::basic_string<char> & buffer, const uint8_t * data, uint8_t length)
{
  auto op = AppendData<NodeCreateOperation>(buffer,
    sizeof(NodeCreateOperation) + length);
  op->type = OperationType::NodeCreateOperation;
  op->nodeTypeLength = length;
  std::memcpy(&op->data, data, length);
}

void OperationBuilder::EdgeCreate(std::basic_string<char> & buffer, const NodeId & parentId, const NodeId & childId)
{
  auto op = AppendData<EdgeCreateOperation>(buffer);
  op->type = OperationType::EdgeCreateOperation;
  op->parentId = parentId;
  op->childId = childId;
}

void OperationBuilder::EdgeDelete(std::basic_string<char> & buffer, const NodeId & parentId, const EdgeId & edgeId)
{
  auto op = AppendData<EdgeDeleteOperation>(buffer);
  op->type = OperationType::EdgeDeleteOperation;
  op->parentId = parentId;
  op->edgeId = edgeId;
}

void OperationBuilder::ValueSet(std::basic_string<char> & buffer, const NodeId & nodeId, const uint8_t * data, uint32_t length)
{
  auto op = AppendData<ValueSetOperation>(buffer,
    sizeof(ValueSetOperation) + length);
  op->type = OperationType::ValueSetOperation;
  op->nodeId = nodeId;
  op->length = length;
  std::memcpy(op->data, data, length);
}

void OperationBuilder::ValueSetPreview(std::basic_string<char> & buffer, const NodeId & nodeId, const uint8_t * data, uint32_t length)
{
  auto op = AppendData<ValueSetOperation>(buffer,
    sizeof(ValueSetOperation) + length);
  op->type = OperationType::ValuePreviewOperation;
  op->nodeId = nodeId;
  op->length = length;
  std::memcpy(op->data, data, length);
}

void OperationBuilder::BlockValueInsertAfter(std::basic_string<char> & buffer, const NodeId & nodeId, const Timestamp & blockId, uint32_t offset, const uint8_t * data, uint32_t length)
{
  auto op = AppendData<BlockValueInsertAfterOperation>(buffer,
    sizeof(BlockValueInsertAfterOperation) + length);
  op->type = OperationType::BlockValueInsertAfterOperation;
  op->nodeId = nodeId;
  op->blockId = blockId;
  op->offset = offset;
  op->length = length;
  std::memcpy(&op->data, data, length);
}

void OperationBuilder::BlockValueDeleteAfter(std::basic_string<char> & buffer, const NodeId & nodeId, const Timestamp & blockId, uint32_t offset, uint32_t length)
{
  auto op = AppendData<BlockValueDeleteAfterOperation>(buffer);
  op->type = OperationType::BlockValueDeleteAfterOperation;
  op->nodeId = nodeId;
  op->blockId = blockId;
  op->offset = offset;
  op->length = length;
}

std::basic_string<char> OperationBuilder::DoubleToValueData(NodeType baseType, double value)
{
  std::basic_string<char> data;

  if (baseType == PrimitiveNodeTypes::BoolValue())
  {
    *AppendData<bool>(data) = value;
  }
  else if (baseType == PrimitiveNodeTypes::DoubleValue())
  {
    *AppendData<double>(data) = value;
  }
  else if (baseType == PrimitiveNodeTypes::FloatValue())
  {
    *AppendData<float>(data) = value;
  }
  else if (baseType == PrimitiveNodeTypes::Int32Value())
  {
    *AppendData<int32_t>(data) = value;
  }
  else if (baseType == PrimitiveNodeTypes::Int64Value())
  {
    *AppendData<int64_t>(data) = value;
  }
  else if (baseType == PrimitiveNodeTypes::Int8Value())
  {
    *AppendData<int8_t>(data) = value;
  }
  else
  {
    //not a supported type
  }

  return data;
}

Timestamp OperationBuilder::getCurrentTimestamp() const
{
  uint32_t clock = this->clock->getMaxClock();

  if (groupContexts.size() > 0)
  {
    if (groupContexts.front().type == OperationType::AtomicGroupOperation)
    {
      clock += 1;
    }
    else
    {
      //for now, assume there are no nested groups
      clock += groupContexts.front().opCount;
    }
  }

  return { clock, siteId };
}

Timestamp OperationBuilder::getNextTimestamp() const
{
  return getCurrentTimestamp() + 1;
}