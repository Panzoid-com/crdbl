#include "Core.h"
#include "Serialization/LogOperationSerialization.h"
#include "Streams/CallbackWritableStream.h"

Core::Core()
{
  setUpBuiltInNodes();
}

Core::Core(CoreInit coreInit)
  : coreInit(coreInit)
{
  setUpBuiltInNodes();
}

Core::~Core()
{
  for (auto it = nodes.begin(); it != nodes.end(); ++it)
  {
    Node * node = it->second;
    NodeType baseType = node->getBaseType();
    auto primitiveType = PrimitiveNodeTypes::nodeTypeToPrimitiveType(baseType);

    switch (primitiveType)
    {
      case PrimitiveNodeTypes::PrimitiveType::Set:
        delete static_cast<SetNode *>(node);
        break;
      case PrimitiveNodeTypes::PrimitiveType::List:
        delete static_cast<ListNode *>(node);
        break;
      case PrimitiveNodeTypes::PrimitiveType::Map:
        delete static_cast<MapNode *>(node);
        break;
      case PrimitiveNodeTypes::PrimitiveType::Reference:
        delete static_cast<ReferenceNode *>(node);
        break;
      case PrimitiveNodeTypes::PrimitiveType::OrderedFloat64Map:
        delete static_cast<OrderedFloat64MapNode *>(node);
        break;
      case PrimitiveNodeTypes::PrimitiveType::Int32Value:
        delete static_cast<ValueNode<int32_t> *>(node);
        break;
      case PrimitiveNodeTypes::PrimitiveType::Int64Value:
        delete static_cast<ValueNode<int64_t> *>(node);
        break;
      case PrimitiveNodeTypes::PrimitiveType::FloatValue:
        delete static_cast<ValueNode<float> *>(node);
        break;
      case PrimitiveNodeTypes::PrimitiveType::DoubleValue:
        delete static_cast<ValueNode<double> *>(node);
        break;
      case PrimitiveNodeTypes::PrimitiveType::Int8Value:
        delete static_cast<ValueNode<int8_t> *>(node);
        break;
      case PrimitiveNodeTypes::PrimitiveType::BoolValue:
        delete static_cast<ValueNode<bool> *>(node);
        break;
      case PrimitiveNodeTypes::PrimitiveType::StringValue:
        delete static_cast<BlockValueNode<char> *>(node);
        break;
      default:
        delete node;
        break;
    }
  }

  for (auto it : nodeTypeReadyPromises)
  {
    it.second.reject();
  }

  for (auto it : nodeReadyPromises)
  {
    it.second.reject();
  }

  for (auto it : getTypeSpecPromises)
  {
    it.second.reject();
  }

  for (auto it : blockValueData)
  {
    delete [] it;
  }
}

void Core::setUpBuiltInNodes()
{
  Node * node;

  node = new Node();
  node->addType(PrimitiveNodeTypes::Null());
  node->effect.initialize();
  nodes[NodeId::Null] = node;

  node = new Node();
  node->addType(PrimitiveNodeTypes::Abstract());
  node->effect.initialize();
  nodes[NodeId::Root] = node;

  node = new Node();
  node->effect.initialize();
  nodes[NodeId::Pending] = node;

  //send an artificial event when the root node is created/ready
  waitForNodeReady(NodeId::SiteRoot).then([this]()
  {
    auto event = new NodeAddedEvent();
    event->parentId = NodeId::Root;
    event->edgeId = EdgeId::Null;
    event->childId = NodeId::SiteRoot;
    event->speculative = false;
    coreInit.eventRaised(*event);
    delete event;
  });
}

char * Core::createBlockValueData(const uint8_t * data, uint32_t length)
{
  //all block value data is owned by the db and freed when the db unloads
  //due to the immutable and usually permanent nature of block value data
  //this is usually fine and avoids needlessly tracking long-lived items
  //however, it could definitely still be improved over this simple approach

  char * dataCopy = new char[length];
  std::memcpy(dataCopy, data, length);
  blockValueData.push_back(dataCopy);
  return dataCopy;
}

const Node * Core::getExistingNode(const NodeId & nodeId) const
{
  Node * node = nullptr;

  auto it = nodes.find(nodeId);
  if (it != nodes.end())
  {
    node = it->second;
  }

  return node;
}

Node * Core::getNode(const NodeId & nodeId)
{
  Node * node = nullptr;

  auto it = nodes.find(nodeId);
  if (it != nodes.end())
  {
    node = it->second;
  }

  if (node == nullptr)
  {
    node = new Node();
    nodes[nodeId] = node;
  }

  return node;
}

template <class T>
Node * Core::swapNode(Node * node)
{
  Node * newNode = new T();
  *newNode = *node;
  delete node;
  return newNode;
}

void Core::assignContainerNodeAttributes(Node * node, const AttributeMap * attributes)
{
  auto containerNode = static_cast<ContainerNode *>(node);
  if (attributes != nullptr)
  {
    containerNode->childType.value =
      NodeType(getAttributeValueOrDefault<std::string>(*attributes, 0));
    containerNode->childType.definedByType = getAttributeDefinedByType(*attributes, 0);
  }
}

void Core::setNodeType(const NodeId & nodeId, const PrimitiveNodeType & type,
  const AttributeMap * attributes)
{
  Node * node = getNode(nodeId);
  auto primitiveType = PrimitiveNodeTypes::nodeTypeToPrimitiveType(type);

  switch (primitiveType)
  {
    case PrimitiveNodeTypes::PrimitiveType::Abstract:
      node = swapNode<Node>(node);
      break;
    case PrimitiveNodeTypes::PrimitiveType::Set:
      node = swapNode<SetNode>(node);
      assignContainerNodeAttributes(node, attributes);
      break;
    case PrimitiveNodeTypes::PrimitiveType::List:
      node = swapNode<ListNode>(node);
      assignContainerNodeAttributes(node, attributes);
      break;
    case PrimitiveNodeTypes::PrimitiveType::Map:
      node = swapNode<MapNode>(node);
      assignContainerNodeAttributes(node, attributes);
      break;
    case PrimitiveNodeTypes::PrimitiveType::Reference:
      node = swapNode<ReferenceNode>(node);
      assignContainerNodeAttributes(node, attributes);
      break;
    case PrimitiveNodeTypes::PrimitiveType::OrderedFloat64Map:
      node = swapNode<OrderedFloat64MapNode>(node);
      assignContainerNodeAttributes(node, attributes);
      break;
    case PrimitiveNodeTypes::PrimitiveType::Int32Value:
      node = swapNode<ValueNode<int32_t>>(node);
      break;
    case PrimitiveNodeTypes::PrimitiveType::Int64Value:
      node = swapNode<ValueNode<int64_t>>(node);
      break;
    case PrimitiveNodeTypes::PrimitiveType::FloatValue:
      node = swapNode<ValueNode<float>>(node);
      break;
    case PrimitiveNodeTypes::PrimitiveType::DoubleValue:
      node = swapNode<ValueNode<double>>(node);
      break;
    case PrimitiveNodeTypes::PrimitiveType::Int8Value:
      node = swapNode<ValueNode<int8_t>>(node);
      break;
    case PrimitiveNodeTypes::PrimitiveType::BoolValue:
      node = swapNode<ValueNode<bool>>(node);
      break;
    case PrimitiveNodeTypes::PrimitiveType::StringValue:
      node = swapNode<BlockValueNode<char>>(node);
      break;
    case PrimitiveNodeTypes::PrimitiveType::Null:
      break;
  }

  nodes[nodeId] = node;
}

bool Core::isNodeTypeReady(const Node * node) const
{
  //a node's type is ready when its full type is known (all inherited node types)
  //and the node's class in memory is initialized to the correct primitive type

  if (PrimitiveNodeTypes::isPrimitiveNodeType(node->getBaseType()) && node->isAbstractType() == false)
  {
    return true;
  }

  return false;
}

bool Core::isNodeReady(const Node * node) const
{
  //a node is ready when all type specs for all of its inherited types have been applied

  if (node != nullptr && node->effect.isVisible())
  {
    return true;
  }

  return false;
}

Promise<void> Core::waitForNodeTypeReady(const NodeId & nodeId)
{
  Node * node = getNode(nodeId);

  if (isNodeTypeReady(node) == true)
  {
    return Promise<void>::Resolve();
  }

  return nodeTypeReadyPromises[nodeId];

}

Promise<void> Core::waitForNodeReady(const NodeId & nodeId)
{
  Node * node = getNode(nodeId);

  if (isNodeReady(node) == true)
  {
    return Promise<void>::Resolve();
  }

  return nodeReadyPromises[nodeId];
}

Promise<std::tuple<const Operation *, size_t>> Core::getTypeSpec(NodeType nodeType)
{
  auto it = getTypeSpecPromises.find(nodeType);
  if (it == getTypeSpecPromises.end())
  {
    typeRequests.push_back(nodeType);
    it = getTypeSpecPromises.emplace(nodeType,
      Promise<std::tuple<const Operation *, size_t>>()).first;
  }

  return it->second;
}

void Core::processTypeRequests()
{
  if (!isProcessingTypeRequests)
  {
    isProcessingTypeRequests = true;
    for (auto it = typeRequests.begin(); it != typeRequests.end(); ++it)
    {
      coreInit.getTypeSpec((*it).toString());
    }
    typeRequests.clear();
    isProcessingTypeRequests = false;
  }
}

void Core::resolveTypeSpec(const std::string & type, const Operation * ops, size_t length)
{
  NodeType _type = NodeType(type);

  auto it = getTypeSpecPromises.find(_type);
  if (it != getTypeSpecPromises.end())
  {
    Promise<std::tuple<const Operation *, size_t>> & prom = it->second;
    prom.resolve(std::make_tuple(ops, length));

    getTypeSpecPromises.erase(it);

    processTypeRequests();
  }
}

Promise<void> Core::inheritType(const NodeId & nodeId, NodeType type,
  InheritanceContext * prevInheritanceContext, AttributeMap * attributes)
{
  Node * node = getNode(nodeId);

  if (prevInheritanceContext == nullptr)
  {
    node->createdByRootOffset = 0;
  }
  else if (prevInheritanceContext->subtreeOffset != *prevInheritanceContext->offset)
  {
    //if this node is inherited from a type, note the root that created it
    node->createdByRootOffset = prevInheritanceContext->subtreeOffset;
  }

  if (PrimitiveNodeTypes::isPrimitiveNodeType(type))
  {
    if (!PrimitiveNodeTypes::isNullNodeType(type))
    {
      //there is only one Null node; change the type to abstract
      //not sure if this is the best choice but this is a rare edge case
      node->addType(type);
    }

    setNodeType(nodeId, static_cast<PrimitiveNodeType>(type), attributes);

    return Promise<void>::Resolve();
  }
  else
  {
    //NOTE: the type appears in the list before inheritance is necessarily complete
    //  not sure if this should be the behavior here
    node->addType(type);
  }

  return getTypeSpec(type).then([this, type, nodeId, prevInheritanceContext, attributes]
    (std::tuple<const Operation *, size_t> ops)
  {
    auto [opsData, length] = ops;

    OperationIterator * it = new OperationIterator(
      reinterpret_cast<const Operation *>(opsData), length);
    InheritanceContext * inheritanceContext;
    if (prevInheritanceContext != nullptr)
    {
      inheritanceContext = new InheritanceContext(*prevInheritanceContext);
    }
    else
    {
      inheritanceContext = new InheritanceContext(nodeId.ts);
      inheritanceContext->attributes = attributes;
    }
    inheritanceContext->type = type;

    return applyOperations(it, inheritanceContext)
      .then([inheritanceContext]()
      {
        inheritanceContext->callback.resolve();
      })
      .finally([it, inheritanceContext]()
      {
        delete inheritanceContext;
        delete it;
      });
  });
}

Promise<void> Core::applyOperations(OperationIterator * it, InheritanceContext * inheritanceContext)
{
  if (inheritanceContext == nullptr)
  {
    //NOTE: shouldn't happen right now
    return Promise<void>::Resolve();
  }

  while (**it != nullptr)
  {
    Timestamp childTs = { 1 + inheritanceContext->operationCount++, 0 };
    Promise<void> prom = applyOperation(childTs, **it, inheritanceContext);
    ++*it;

    if (!prom.isSettled())
    {
      return prom.then([this, it, inheritanceContext]()
      {
        return applyOperations(it, inheritanceContext);
      });
    }
  }

  return Promise<void>::Resolve();
}

void Core::applyOperation(const RefCounted<const LogOperation> & op)
{
  auto promise = applyOperation(op->ts, &op->op, (InheritanceContext *)nullptr);
  if (!promise.isSettled())
  {
    //this is a bit of an awkward (but valid) way to hold a reference
    RefCounted<const LogOperation> holdRef(op);
    promise.then([op = std::move(op)](){});
  }

  processTypeRequests();
}

IWritableStream<RefCounted<const LogOperation>> * Core::createApplyStream()
{
  auto * callbackStream = new CallbackWritableStream<RefCounted<const LogOperation>>(
    [&](const RefCounted<const LogOperation> & op)
    {
      applyOperation(op);
    },
    []()
    {

    });

  return callbackStream;
}

void Core::unapplyOperation(const RefCounted<const LogOperation> & op)
{
  unapplyOperation(op->ts, &op->op);
}

IWritableStream<RefCounted<const LogOperation>> * Core::createUnapplyStream()
{
  auto * callbackStream = new CallbackWritableStream<RefCounted<const LogOperation>>(
    [&](const RefCounted<const LogOperation> & op)
    {
      unapplyOperation(op);
    },
    []()
    {

    });

  return callbackStream;
}

Promise<void> Core::applyOperation(const Timestamp & ts, const Operation * op, InheritanceContext * inheritanceContext)
{
  if (inheritanceContext == nullptr &&
    !op->isGroup() &&
    op->type != OperationType::ValuePreviewOperation)
  {
    clock.update(ts);
  }

  switch (op->type)
  {
    case OperationType::NoOpOperation:
      return Promise<void>::Resolve();

    case OperationType::UndoGroupOperation:
    case OperationType::RedoGroupOperation:
      return applyOperation(ts, static_cast<const UndoGroupOperation *>(op), inheritanceContext);

    case OperationType::GroupOperation:
      return applyOperation(ts, static_cast<const GroupOperation *>(op), inheritanceContext);
    case OperationType::AtomicGroupOperation:
      return applyOperation(ts, static_cast<const AtomicGroupOperation *>(op), inheritanceContext);

    case OperationType::NodeCreateOperation:
      return applyOperation(ts, static_cast<const NodeCreateOperation *>(op), inheritanceContext, nullptr);

    case OperationType::EdgeCreateOperation:
      return applyOperation(ts, static_cast<const EdgeCreateOperation *>(op), inheritanceContext, nullptr);
    case OperationType::EdgeDeleteOperation:
      return applyOperation(ts, static_cast<const EdgeDeleteOperation *>(op), inheritanceContext);

    case OperationType::ValuePreviewOperation:
    case OperationType::ValueSetOperation:
      return applyOperation(ts, static_cast<const ValueSetOperation *>(op), inheritanceContext);
    case OperationType::BlockValueInsertAfterOperation:
      return applyOperation(ts, static_cast<const BlockValueInsertAfterOperation *>(op), inheritanceContext);
    case OperationType::BlockValueDeleteAfterOperation:
      return applyOperation(ts, static_cast<const BlockValueDeleteAfterOperation *>(op), inheritanceContext);

    default:
      return Promise<void>::Resolve();
  }
}

Promise<void> Core::applyUndoOperation(const Timestamp & ts, const Timestamp & prevTs,
  const Operation * op, bool isUndo)
{
  clock.update(ts);

  switch (op->type)
  {
    case OperationType::EdgeCreateOperation:
    case OperationType::UndoEdgeCreateOperation:
      return applyUndoOperation(ts, prevTs, static_cast<const UndoEdgeCreateOperation *>(op), isUndo);
    case OperationType::EdgeDeleteOperation:
    case OperationType::UndoEdgeDeleteOperation:
      return applyUndoOperation(ts, prevTs, static_cast<const UndoEdgeDeleteOperation *>(op), isUndo);

    case OperationType::ValueSetOperation:
    case OperationType::UndoValueSetOperation:
      return applyUndoOperation(ts, prevTs, static_cast<const UndoValueSetOperation *>(op), isUndo);
    case OperationType::BlockValueInsertAfterOperation:
    case OperationType::UndoBlockValueInsertAfterOperation:
      return applyUndoOperation(ts, prevTs, static_cast<const UndoBlockValueInsertAfterOperation *>(op), isUndo);
    case OperationType::BlockValueDeleteAfterOperation:
    case OperationType::UndoBlockValueDeleteAfterOperation:
      return applyUndoOperation(ts, prevTs, static_cast<const UndoBlockValueDeleteAfterOperation *>(op), isUndo);

    default:
      return Promise<void>::Resolve();
  }
}

void Core::unapplyOperation(const Timestamp & ts, const Operation * op)
{
  switch (op->type)
  {
    case OperationType::UndoGroupOperation:
    case OperationType::RedoGroupOperation:
      return unapplyOperation(ts, static_cast<const UndoGroupOperation *>(op));
    case OperationType::GroupOperation:
      return unapplyOperation(ts, static_cast<const GroupOperation *>(op));
    case OperationType::AtomicGroupOperation:
      return unapplyOperation(ts, static_cast<const AtomicGroupOperation *>(op));

    case OperationType::NodeCreateOperation:
      return unapplyOperation(ts, static_cast<const NodeCreateOperation *>(op));

    case OperationType::EdgeCreateOperation:
      return unapplyOperation(ts, static_cast<const EdgeCreateOperation *>(op));
    case OperationType::EdgeDeleteOperation:
      return unapplyOperation(ts, static_cast<const EdgeDeleteOperation *>(op));

    case OperationType::ValuePreviewOperation:
    case OperationType::ValueSetOperation:
      return unapplyOperation(ts, static_cast<const ValueSetOperation *>(op));
    case OperationType::BlockValueInsertAfterOperation:
      return unapplyOperation(ts, static_cast<const BlockValueInsertAfterOperation *>(op));
    case OperationType::BlockValueDeleteAfterOperation:
      return unapplyOperation(ts, static_cast<const BlockValueDeleteAfterOperation *>(op));

    default:
      return;
  }
}

void Core::unapplyUndoOperation(const Timestamp & ts, const Timestamp & prevTs, const Operation * op, bool isUndo)
{
  switch (op->type)
  {
    case OperationType::EdgeCreateOperation:
    case OperationType::UndoEdgeCreateOperation:
      return unapplyUndoOperation(ts, prevTs, static_cast<const UndoEdgeCreateOperation *>(op), isUndo);
    case OperationType::EdgeDeleteOperation:
    case OperationType::UndoEdgeDeleteOperation:
      return unapplyUndoOperation(ts, prevTs, static_cast<const UndoEdgeDeleteOperation *>(op), isUndo);

    case OperationType::ValueSetOperation:
    case OperationType::UndoValueSetOperation:
      return unapplyUndoOperation(ts, prevTs, static_cast<const UndoValueSetOperation *>(op), isUndo);
    case OperationType::BlockValueInsertAfterOperation:
    case OperationType::UndoBlockValueInsertAfterOperation:
      return unapplyUndoOperation(ts, prevTs, static_cast<const UndoBlockValueInsertAfterOperation *>(op), isUndo);
    case OperationType::BlockValueDeleteAfterOperation:
    case OperationType::UndoBlockValueDeleteAfterOperation:
      return unapplyUndoOperation(ts, prevTs, static_cast<const UndoBlockValueDeleteAfterOperation *>(op), isUndo);

    default:
      return;
  }
}

NodeId Core::transformNodeId(InheritanceContext * inheritanceContext, const NodeId & nodeId)
{
  if (inheritanceContext == nullptr)
  {
    return nodeId;
  }

  return inheritanceContext->transformNodeId(nodeId);
}

EdgeId Core::transformEdgeId(InheritanceContext * inheritanceContext, const NodeId & edgeId)
{
  if (inheritanceContext == nullptr)
  {
    return edgeId;
  }

  return inheritanceContext->transformEdgeId(edgeId);
}

Timestamp Core::transformTimestamp(InheritanceContext * inheritanceContext, const Timestamp & timestamp)
{
  if (inheritanceContext == nullptr)
  {
    return timestamp;
  }

  return inheritanceContext->transformTimestamp(timestamp);
}

Timestamp Core::transformTimestamp(InheritanceContext * inheritanceContext, const Timestamp & blockId, const Timestamp & timestamp)
{
  if (inheritanceContext == nullptr)
  {
    return timestamp;
  }

  return blockId + timestamp.clock;
}

Timestamp Core::transformBlockId(InheritanceContext * inheritanceContext, const NodeId & nodeId, const Timestamp & blockId)
{
  if (inheritanceContext == nullptr)
  {
    return blockId;
  }

  if (nodeId.ts == blockId)
  {
    return inheritanceContext->transformTimestamp(blockId);
  }

  return blockId;
}

Promise<void> Core::applyOperation(const Timestamp & ts, const UndoGroupOperation * op, InheritanceContext * inheritanceContext)
{
  if (inheritanceContext != nullptr)
  {
    return Promise<void>::Resolve();
  }

  bool isUndo = op->type == OperationType::UndoGroupOperation;

  OperationIterator it(reinterpret_cast<const Operation *>(op->data), op->length);
  PromiseAll * allResolved = new PromiseAll();

  Timestamp childTs = ts;
  Timestamp childPrevTs = op->prevTs;

  while (*it)
  {
    allResolved->add(applyUndoOperation(childTs, childPrevTs, *it, isUndo));
    ++it;
    ++childTs.clock;
    ++childPrevTs.clock;
  }

  return allResolved->getPromise().finally([allResolved]()
  {
    delete allResolved;
  });
}

Promise<void> Core::applyOperation(const Timestamp & ts, const GroupOperation * op, InheritanceContext * inheritanceContext)
{
  OperationIterator it(reinterpret_cast<const Operation *>(op->data), op->length);
  PromiseAll * allResolved = new PromiseAll();

  Timestamp childTs = ts;

  if (inheritanceContext == nullptr)
  {
    while (*it)
    {
      allResolved->add(applyOperation(childTs, *it, (InheritanceContext *)nullptr));
      ++it;
      ++childTs.clock;
    }
  }
  else
  {
    // do nothing; groups are not allowed in inheritance contexts
  }

  return allResolved->getPromise().finally([allResolved]()
  {
    delete allResolved;
  });
}

Promise<void> Core::applyOperation(const Timestamp & ts, const AtomicGroupOperation * op, InheritanceContext * inheritanceContext)
{
  OperationIterator it(reinterpret_cast<const Operation *>(op->data), op->length);
  Promise<void> result = Promise<void>::Resolve();

  const Operation * createOp = *it;

  if (createOp == nullptr)
  {
    //malformed or invalid atomic group
    return result;
  }
  else if (createOp->type != OperationType::NodeCreateOperation &&
    createOp->type != OperationType::EdgeCreateOperation)
  {
    //atomic group must start with node/edge create operation
    return result;
  }

  ++it;

  AttributeMap * attributes = nullptr;
  NodeType definedByType = PrimitiveNodeTypes::Null();
  bool attributesNeedsFree = false;

  if (inheritanceContext != nullptr)
  {
    definedByType = inheritanceContext->type;

    if (ts.isTypeRoot() &&
    inheritanceContext->attributes != nullptr)
    {
      attributes = inheritanceContext->attributes;
    }
  }

  if (attributes == nullptr)
  {
    attributes = new AttributeMap();
    attributesNeedsFree = true;

    if (inheritanceContext != nullptr && ts.isTypeRoot())
    {
      inheritanceContext->attributes = attributes;
    }
  }

  while (*it)
  {
    auto attributeOp = reinterpret_cast<const SetAttributeOperation *>(*it);
    if (attributeOp->type == OperationType::SetAttributeOperation)
    {
      setAttributeValue(*attributes, attributeOp->attributeId, definedByType,
        reinterpret_cast<const char *>(&attributeOp->data),
        static_cast<size_t>(attributeOp->length));
    }
    ++it;
  }

  if (inheritanceContext == nullptr)
  {
    clock.update(ts);
  }

  if (createOp->type == OperationType::NodeCreateOperation)
  {
    auto nodeCreateOp = reinterpret_cast<const NodeCreateOperation *>(createOp);
    result = applyOperation(ts, nodeCreateOp, inheritanceContext, attributes);
  }
  else if (createOp->type == OperationType::EdgeCreateOperation)
  {
    auto edgeCreateOp = reinterpret_cast<const EdgeCreateOperation *>(createOp);
    result = applyOperation(ts, edgeCreateOp, inheritanceContext, attributes);
  }

  return result.finally([attributes, attributesNeedsFree]()
  {
    if (attributesNeedsFree)
    {
      delete attributes;
    }
  });
}

Promise<void> Core::applyOperation(const Timestamp & ts, const NodeCreateOperation * op,
  InheritanceContext * inheritanceContext, AttributeMap * attributes)
{
  NodeId nodeId = NodeId::inheritanceRootFor(ts);
  nodeId = transformNodeId(inheritanceContext, nodeId);
  NodeType nodeType = NodeType(std::string((const char *)op->data, (size_t)op->nodeTypeLength));

  //NOTE: this is a bit primitive
  //  we need at least some basic mechanism to prevent creating nodes twice
  //  future iterations will refine this mechanism
  const Node * existingNode = getExistingNode(nodeId);
  if (existingNode != nullptr && existingNode->effect.isInitialized())
  {
    return Promise<void>::Resolve();
  }

  AttributeMap * attr;
  if (inheritanceContext != nullptr && ts.isTypeRoot() &&
    inheritanceContext->attributes != nullptr)
  {
    attr = inheritanceContext->attributes;
  }
  else
  {
    attr = attributes;
  }

  Promise<void> ret = inheritType(nodeId, nodeType, inheritanceContext, attr); //NOTE: can change node pointers

  if (PrimitiveNodeTypes::isPrimitiveNodeType(nodeType))
  {
    //resolve type ready promises once the node's type is fully defined
    auto it = nodeTypeReadyPromises.find(nodeId);
    if (it != nodeTypeReadyPromises.end())
    {
      it->second.resolve();
      nodeTypeReadyPromises.erase(it);
    }
  }

  //handle string value inheritance (in place construction)
  //this can go somewhere else if more appropriate
  if (nodeType == PrimitiveNodeTypes::StringValue() && inheritanceContext != nullptr)
  {
    InheritanceContext * list = inheritanceContext;
    while (list->parent != nullptr)
    {
      list = list->parent;
    }

    Node * rootNode = getNode({ list->root, 0 });
    auto cacheKey = std::make_pair(rootNode->getType(), *list->offset);

    auto it = blockValueCache.find(cacheKey);
    if (it == blockValueCache.end())
    {
      blockValueCache.emplace(cacheKey, BlockValueCacheItem(list->root));

      InheritanceContext * context = inheritanceContext;
      while (context != nullptr)
      {
        context->callback.then([this, cacheKey]()
        {
          blockValueCache[cacheKey].indexOffset = 0;
        });

        context = context->parent;
      }
    }

    list->callback.then([this, nodeId, cacheKey, list]()
    {
      auto & value = blockValueCache[cacheKey].value;
      if (value.size() > 0)
      {
        auto blockValueNode = static_cast<BlockValueNode<char> *>(getNode(nodeId));
        blockValueNode->value.insertAfter(Timestamp::Null, 0, list->root,
          value.size(), value.data(), [](size_t, char *, uint32_t){});
      }
    });
  }

  if (inheritanceContext == nullptr)
  {
    return ret.then([this, nodeId]()
    {
      Node * node = getNode(nodeId);

      node->effect.initialize();

      //resolve ready promises once the node's types are fully inherited
      auto it = nodeReadyPromises.find(nodeId);
      if (it != nodeReadyPromises.end())
      {
        it->second.resolve();
        nodeReadyPromises.erase(it);
      }
    });
  }

  return ret;
}

Promise<void> Core::applyOperation(const Timestamp & ts, const EdgeCreateOperation * op,
  InheritanceContext * inheritanceContext, const AttributeMap * attributes)
{
  NodeId parentId = transformNodeId(inheritanceContext, op->parentId);
  NodeId childId = transformNodeId(inheritanceContext, op->childId);
  EdgeId edgeId = transformEdgeId(inheritanceContext, NodeId::inheritanceRootFor(ts));

  bool inherited = inheritanceContext != nullptr;

  std::function<void(EdgeEvent &)> changedCallback = [this, parentId, inherited](EdgeEvent & event)
  {
    if (inherited)
      return;

    if (isNodeReady(getExistingNode({ parentId.ts, 0 })) == false)
    {
      return;
    }

    event.parentId = parentId;
    coreInit.eventRaised(event);
  };

  auto parentReadyCallback = [this, parentId, childId, edgeId, inheritanceContext,
    attributes, changedCallback]()
  {
    Node * parent = getNode(parentId);
    Node * child = getNode(childId);
    NodeType parentBaseType = parent->getBaseType();

    if (PrimitiveNodeTypes::isContainerNodeType(parentBaseType) == false)
    {
      //parent is not a container
      return Promise<void>::Resolve();
    }

    bool nodeReady;
    if (inheritanceContext == nullptr)
    {
      nodeReady = isNodeReady(getExistingNode({ childId.ts, 0 }));
    }
    else
    {
      nodeReady = isNodeTypeReady(child);
    }

    if (nodeReady == false)
    {
      //only set value speculatively if the node is not yet ready

      Edge * createdEdge = nullptr;
      auto parentPrimitiveType = PrimitiveNodeTypes::nodeTypeToPrimitiveType(parentBaseType);

      switch (parentPrimitiveType)
      {
        case PrimitiveNodeTypes::PrimitiveType::Set:
        {
          auto containerNode = static_cast<SetNode *>(parent);
          createdEdge = containerNode->createEdge(edgeId, NodeId::Pending, attributes, changedCallback);
          break;
        }
        case PrimitiveNodeTypes::PrimitiveType::List:
        {
          auto containerNode = static_cast<ListNode *>(parent);
          EdgeId prevEdgeId;
          if (attributes != nullptr)
          {
            prevEdgeId = transformEdgeId(inheritanceContext,
              getAttributeValueOrDefault<EdgeId>(*attributes, 0));
          }
          else
          {
            prevEdgeId = { {0, 0}, 0 };
          }
          createdEdge = containerNode->createEdge(edgeId, NodeId::Pending, prevEdgeId, attributes, changedCallback);
          break;
        }
        case PrimitiveNodeTypes::PrimitiveType::Map:
        {
          auto containerNode = static_cast<MapNode *>(parent);
          createdEdge = containerNode->createEdge(edgeId, NodeId::Pending, attributes, changedCallback);
          break;
        }
        case PrimitiveNodeTypes::PrimitiveType::Reference:
        {
          auto containerNode = static_cast<ReferenceNode *>(parent);
          createdEdge = containerNode->createEdge(edgeId, NodeId::Pending, attributes, changedCallback);
          break;
        }
        case PrimitiveNodeTypes::PrimitiveType::OrderedFloat64Map:
        {
          auto containerNode = static_cast<OrderedFloat64MapNode *>(parent);
          createdEdge = containerNode->createEdge(edgeId, NodeId::Pending, attributes, changedCallback);
          break;
        }
        default:
          //parent is not a container
          break;
      }

      if (createdEdge != nullptr)
      {
        if (inheritanceContext == nullptr)
        {
          createdEdge->createdByRootOffset = 0;
        }
        else
        {
          createdEdge->createdByRootOffset = inheritanceContext->subtreeOffset;
        }
      }
    }

    std::function<void()> nodeReadyCallback = [this, parentId, childId, edgeId,
      inheritanceContext, attributes, changedCallback, nodeReady]()
    {
      Node * parent = getNode(parentId);
      Node * child = getNode(childId);
      NodeType parentBaseType = parent->getBaseType();
      auto parentPrimitiveType = PrimitiveNodeTypes::nodeTypeToPrimitiveType(parentBaseType);

      auto containerNode = static_cast<ContainerNode *>(parent);

      bool canCreateEdge = false;
      if (parentPrimitiveType == PrimitiveNodeTypes::PrimitiveType::Reference)
      {
        auto referenceNode = static_cast<ReferenceNode *>(parent);
        canCreateEdge = child->isDerivedFromType(referenceNode->childType.getValueOrDefault()) ||
          (referenceNode->nullable && PrimitiveNodeTypes::isNullNodeType(child->getBaseType()));
      }
      else
      {
        canCreateEdge = child->isDerivedFromType(containerNode->childType.getValueOrDefault());
      }

      if (canCreateEdge == false)
      {
        //wrong type: node can't be added to the parent

        if (nodeReady == false)
        {
          //delete edge

          switch (parentPrimitiveType)
          {
            case PrimitiveNodeTypes::PrimitiveType::Set:
            {
              auto containerNode = static_cast<SetNode *>(parent);
              containerNode->deleteEdge(edgeId, changedCallback);
              break;
            }
            case PrimitiveNodeTypes::PrimitiveType::List:
            {
              auto containerNode = static_cast<ListNode *>(parent);
              containerNode->deleteEdge(edgeId, changedCallback);
              break;
            }
            case PrimitiveNodeTypes::PrimitiveType::Map:
            {
              auto containerNode = static_cast<MapNode *>(parent);
              containerNode->deleteEdge(edgeId, changedCallback);
              break;
            }
            case PrimitiveNodeTypes::PrimitiveType::Reference:
            {
              auto containerNode = static_cast<ReferenceNode *>(parent);
              containerNode->deleteEdge(edgeId, changedCallback);
              break;
            }
            case PrimitiveNodeTypes::PrimitiveType::OrderedFloat64Map:
            {
              auto containerNode = static_cast<OrderedFloat64MapNode *>(parent);
              containerNode->deleteEdge(edgeId, changedCallback);
              break;
            }
            default:
              //parent is not a container
              break;
          }
        }
      }
      else if (nodeReady == true)
      {
        //add node normally

        Edge * createdEdge = nullptr;

        switch (parentPrimitiveType)
        {
          case PrimitiveNodeTypes::PrimitiveType::Set:
          {
            auto containerNode = static_cast<SetNode *>(parent);
            createdEdge = containerNode->createEdge(edgeId, childId, attributes, changedCallback);
            break;
          }
          case PrimitiveNodeTypes::PrimitiveType::List:
          {
            auto containerNode = static_cast<ListNode *>(parent);
            EdgeId prevEdgeId;
            if (attributes != nullptr)
            {
              prevEdgeId = transformEdgeId(inheritanceContext,
                getAttributeValueOrDefault<EdgeId>(*attributes, 0));
            }
            else
            {
              prevEdgeId = { {0, 0}, 0 };
            }
            createdEdge = containerNode->createEdge(edgeId, childId, prevEdgeId, attributes, changedCallback);
            break;
          }
          case PrimitiveNodeTypes::PrimitiveType::Map:
          {
            auto containerNode = static_cast<MapNode *>(parent);
            createdEdge = containerNode->createEdge(edgeId, childId, attributes, changedCallback);
            break;
          }
          case PrimitiveNodeTypes::PrimitiveType::Reference:
          {
            auto containerNode = static_cast<ReferenceNode *>(parent);
            createdEdge = containerNode->createEdge(edgeId, childId, attributes, changedCallback);
            break;
          }
          case PrimitiveNodeTypes::PrimitiveType::OrderedFloat64Map:
          {
            auto containerNode = static_cast<OrderedFloat64MapNode *>(parent);
            createdEdge = containerNode->createEdge(edgeId, childId, attributes, changedCallback);
            break;
          }
          default:
            //parent is not a container
            break;
          }

        if (createdEdge != nullptr)
        {
          if (inheritanceContext == nullptr)
          {
            createdEdge->createdByRootOffset = 0;
          }
          else
          {
            createdEdge->createdByRootOffset = inheritanceContext->subtreeOffset;
          }
        }
      }
      else
      {
        //init speculatively added node

        switch (parentPrimitiveType)
        {
          case PrimitiveNodeTypes::PrimitiveType::Set:
          {
            auto containerNode = static_cast<SetNode *>(parent);
            containerNode->initEdge(edgeId, childId, changedCallback);
            break;
          }
          case PrimitiveNodeTypes::PrimitiveType::List:
          {
            auto containerNode = static_cast<ListNode *>(parent);
            containerNode->initEdge(edgeId, childId, changedCallback);
            break;
          }
          case PrimitiveNodeTypes::PrimitiveType::Map:
          {
            auto containerNode = static_cast<MapNode *>(parent);
            containerNode->initEdge(edgeId, childId, changedCallback);
            break;
          }
          case PrimitiveNodeTypes::PrimitiveType::Reference:
          {
            auto containerNode = static_cast<ReferenceNode *>(parent);
            containerNode->initEdge(edgeId, childId, changedCallback);
            break;
          }
          case PrimitiveNodeTypes::PrimitiveType::OrderedFloat64Map:
          {
            auto containerNode = static_cast<OrderedFloat64MapNode *>(parent);
            containerNode->initEdge(edgeId, childId, changedCallback);
            break;
          }
          default:
            //parent is not a container
            break;
        }
      }
    };

    //only wait for node to be fully ready when not in an inheritance context
    if (inheritanceContext == nullptr)
    {
      return waitForNodeReady({ childId.ts, 0 }).then(nodeReadyCallback);
    }
    else
    {
      return waitForNodeTypeReady(childId).then(nodeReadyCallback);
    }
  };

  return waitForNodeTypeReady(parentId).then(parentReadyCallback);
}

Promise<void> Core::applyOperation(const Timestamp & ts, const EdgeDeleteOperation * op, InheritanceContext * inheritanceContext)
{
  NodeId parentId = transformNodeId(inheritanceContext, op->parentId);
  EdgeId edgeId = transformNodeId(inheritanceContext, op->edgeId);

  bool inherited = inheritanceContext != nullptr;

  return waitForNodeTypeReady(parentId).then([this, parentId, edgeId, inherited]()
  {
    Node * node = getNode(parentId);
    NodeType baseType = node->getBaseType();
    auto primitiveType = PrimitiveNodeTypes::nodeTypeToPrimitiveType(baseType);

    std::function<void(EdgeEvent &)> changedCallback = [this, parentId, inherited](EdgeEvent & event)
    {
      if (inherited == true) return;

      event.parentId = parentId;
      coreInit.eventRaised(event);
    };

    int effect = -1; //(op->type == OperationType::UndoEdgeDeleteOperation) ? 1 : -1;

    auto updateNode = [&](auto * nodeType) {
      nodeType->updateEdgeEffect(edgeId, effect, inherited, changedCallback);
    };

    switch (primitiveType)
    {
      case PrimitiveNodeTypes::PrimitiveType::Set:
        updateNode(static_cast<SetNode *>(node));
        break;
      case PrimitiveNodeTypes::PrimitiveType::List:
        updateNode(static_cast<ListNode *>(node));
        break;
      case PrimitiveNodeTypes::PrimitiveType::Map:
        updateNode(static_cast<MapNode *>(node));
        break;
      case PrimitiveNodeTypes::PrimitiveType::Reference:
        updateNode(static_cast<ReferenceNode *>(node));
        break;
      case PrimitiveNodeTypes::PrimitiveType::OrderedFloat64Map:
        updateNode(static_cast<OrderedFloat64MapNode *>(node));
        break;
      default:
        //not a container node
        break;
    }
  });
}

//static
template<typename T>
int Core::valueNodeChangedCallback(Core * core, const NodeId & nodeId,
  bool generateEvent, const T & newValue, const T & oldValue)
{
  if (generateEvent == false) return 0;

  if (!core->isNodeReady(core->getExistingNode({ nodeId.ts, 0 })))
  {
    return 0;
  }

  NodeValueChangedEvent<T> event;
  event.nodeId = nodeId;
  event.newValue = newValue;
  event.oldValue = oldValue;
  core->coreInit.eventRaised(event);

  return 0;
}

//static
int Core::blockValueNodeChangedCallback(Core * core, const Timestamp & ts,
  const NodeId & nodeId, size_t offset, const char * data, uint32_t length)
{
  if (!core->isNodeReady(core->getExistingNode({ nodeId.ts, 0 })))
  {
    return 0;
  }

  if (data == nullptr)
  {
    NodeBlockValueDeletedEvent event;
    event.ts = ts;
    event.nodeId = nodeId;
    event.offset = offset;
    event.length = length;
    core->coreInit.eventRaised(event);
  }
  else
  {
    NodeBlockValueInsertedEvent event;
    event.ts = ts;
    event.nodeId = nodeId;
    event.offset = offset;
    event.str = data;
    event.length = length;
    core->coreInit.eventRaised(event);
  }

  return 0;
}

Promise<void> Core::applyOperation(const Timestamp & ts, const ValueSetOperation * op, InheritanceContext * inheritanceContext)
{
  NodeId nodeId = transformNodeId(inheritanceContext, op->nodeId);
  Timestamp tts = transformTimestamp(inheritanceContext, ts);

  bool generateEvent = inheritanceContext == nullptr;

  return waitForNodeTypeReady(nodeId).then([this, nodeId, tts, op, generateEvent]()
  {
    Node * node = getNode(nodeId);
    NodeType baseType = node->getBaseType();
    auto primitiveType = PrimitiveNodeTypes::nodeTypeToPrimitiveType(baseType);

    auto updateNode = [&](auto * nodeType) {
      auto callback = [this, nodeId, generateEvent](auto newValue, auto oldValue) {
        Core::valueNodeChangedCallback<decltype(newValue)>(this, nodeId, generateEvent, newValue, oldValue);
      };
      auto valueNode = static_cast<decltype(nodeType)>(node);
      valueNode->value.setValue(tts, op->data, op->length, callback);
    };

    switch (primitiveType)
    {
      case PrimitiveNodeTypes::PrimitiveType::BoolValue:
        updateNode(static_cast<ValueNode<bool> *>(node));
        break;
      case PrimitiveNodeTypes::PrimitiveType::DoubleValue:
        updateNode(static_cast<ValueNode<double> *>(node));
        break;
      case PrimitiveNodeTypes::PrimitiveType::FloatValue:
        updateNode(static_cast<ValueNode<float> *>(node));
        break;
      case PrimitiveNodeTypes::PrimitiveType::Int32Value:
        updateNode(static_cast<ValueNode<int32_t> *>(node));
        break;
      case PrimitiveNodeTypes::PrimitiveType::Int64Value:
        updateNode(static_cast<ValueNode<int64_t> *>(node));
        break;
      case PrimitiveNodeTypes::PrimitiveType::Int8Value:
        updateNode(static_cast<ValueNode<int8_t> *>(node));
        break;
      default:
        //not a value node
        break;
    }
  });
}

Promise<void> Core::applyOperation(const Timestamp & ts, const BlockValueInsertAfterOperation * op, InheritanceContext * inheritanceContext)
{
  //validation: not sure if this is the best place
  //most invalid data has undefined (but safe) behavior, but length 0 is a broken operation
  if (op->length == 0)
  {
    return Promise<void>::Resolve();
  }

  NodeId nodeId = transformNodeId(inheritanceContext, op->nodeId);
  Timestamp blockId = transformBlockId(inheritanceContext, op->nodeId, op->blockId);
  //arbitrary timestamp transformation for inherited blocks
  //these blocks will be consolidated anyway, but the timestamps must be greater than the main blockId
  //or else they will not be ordered correctly (due to being "older" than the original block)
  Timestamp tts = transformTimestamp(inheritanceContext, blockId, ts);

  return waitForNodeTypeReady(nodeId).then([this, nodeId, blockId, tts, op, inheritanceContext]()
  {
    Node * node = getNode(nodeId);
    NodeType baseType = node->getBaseType();

    auto callback = [this, tts, nodeId](auto offset, auto data, auto length) {
      Core::blockValueNodeChangedCallback(this, tts, nodeId, offset, data, length);
    };

    if (baseType == PrimitiveNodeTypes::StringValue())
    {
      if (inheritanceContext != nullptr)
      {
        InheritanceContext * list = inheritanceContext;
        while (list->parent != nullptr)
        {
          list = list->parent;
        }

        Node * rootNode = getNode({ list->root, 0 });
        auto cacheKey = std::make_pair(rootNode->getType(), nodeId.child);
        auto & cacheValue = blockValueCache[cacheKey];
        if (cacheValue.firstWrite == list->root)
        {
          size_t offset = op->offset + cacheValue.indexOffset;
          if (offset > cacheValue.value.size())
          {
            offset = cacheValue.value.size();
          }
          cacheValue.value.insert(offset, reinterpret_cast<const char *>(op->data), op->length);
          if (!(op->blockId == Timestamp::Null))
          {
            cacheValue.indexOffset += op->length;
          }
        }

        return;
      }

      auto blockValueNode = static_cast<BlockValueNode<char> *>(node);
      char * data = createBlockValueData(op->data, op->length);

      blockValueNode->value.insertAfter(blockId, op->offset, tts, op->length,
        data, callback);
    }
    else
    {
      //not a block value node
    }
  });
}

Promise<void> Core::applyOperation(const Timestamp & ts, const BlockValueDeleteAfterOperation * op, InheritanceContext * inheritanceContext)
{
  //validation: not sure if this is the best place
  //most invalid data has undefined (but safe) behavior, but length 0 is a broken operation
  if (op->length == 0)
  {
    return Promise<void>::Resolve();
  }

  NodeId nodeId = transformNodeId(inheritanceContext, op->nodeId);
  Timestamp blockId = transformBlockId(inheritanceContext, op->nodeId, op->blockId);

  return waitForNodeTypeReady(nodeId).then([this, ts, nodeId, blockId, op, inheritanceContext]()
  {
    Node * node = getNode(nodeId);
    NodeType baseType = node->getBaseType();

    auto callback = [this, ts, nodeId](auto offset, auto data, auto length) {
      Core::blockValueNodeChangedCallback(this, ts, nodeId, offset, data, length);
    };

    if (baseType == PrimitiveNodeTypes::StringValue())
    {
      if (inheritanceContext != nullptr)
      {
        InheritanceContext * list = inheritanceContext;
        while (list->parent != nullptr)
        {
          list = list->parent;
        }

        Node * rootNode = getNode({ list->root, 0 });
        auto cacheKey = std::make_pair(rootNode->getType(), nodeId.child);
        auto & cacheValue = blockValueCache[cacheKey];
        if (cacheValue.firstWrite == list->root)
        {
          cacheValue.value.erase(op->offset + cacheValue.indexOffset, op->length);
          cacheValue.indexOffset -= op->length;
        }

        return;
      }

      auto blockValueNode = static_cast<BlockValueNode<char> *>(node);
      blockValueNode->value.updateEffect(blockId, op->offset, op->length, -1, callback);
    }
    else
    {
      //not a block value node
    }
  });
}

Promise<void> Core::applyUndoOperation(const Timestamp & ts, const Timestamp & prevTs,
  const UndoEdgeCreateOperation * op, bool isUndo)
{
  return waitForNodeTypeReady(op->parentId).then([this, prevTs, op, isUndo]()
  {
    Node * node = getNode(op->parentId);
    NodeType baseType = node->getBaseType();
    auto primitiveType = PrimitiveNodeTypes::nodeTypeToPrimitiveType(baseType);

    std::function<void(EdgeEvent &)> changedCallback = [this, op](EdgeEvent & event)
    {
      event.parentId = op->parentId;
      coreInit.eventRaised(event);
    };

    int effect = (isUndo) ? -1 : 1;

    auto updateNode = [&](auto * nodeType) {
      nodeType->updateEdgeEffect(NodeId::inheritanceRootFor(prevTs), effect, false, changedCallback);
    };

    switch (primitiveType)
    {
      case PrimitiveNodeTypes::PrimitiveType::Set:
        updateNode(static_cast<SetNode *>(node));
        break;
      case PrimitiveNodeTypes::PrimitiveType::List:
        updateNode(static_cast<ListNode *>(node));
        break;
      case PrimitiveNodeTypes::PrimitiveType::Map:
        updateNode(static_cast<MapNode *>(node));
        break;
      case PrimitiveNodeTypes::PrimitiveType::Reference:
        updateNode(static_cast<ReferenceNode *>(node));
        break;
      case PrimitiveNodeTypes::PrimitiveType::OrderedFloat64Map:
        updateNode(static_cast<OrderedFloat64MapNode *>(node));
        break;
      default:
        //not a container node
        break;
    }
  });
}

Promise<void> Core::applyUndoOperation(const Timestamp & ts, const Timestamp & prevTs,
  const UndoEdgeDeleteOperation * op, bool isUndo)
{
  return waitForNodeTypeReady(op->parentId).then([this, op, isUndo]()
  {
    Node * node = getNode(op->parentId);
    NodeType baseType = node->getBaseType();
    auto primitiveType = PrimitiveNodeTypes::nodeTypeToPrimitiveType(baseType);

    std::function<void(EdgeEvent &)> changedCallback = [this, op](EdgeEvent & event)
    {
      event.parentId = op->parentId;
      coreInit.eventRaised(event);
    };

    int effect = (isUndo) ? 1 : -1;

    auto updateNode = [&](auto * nodeType) {
      nodeType->updateEdgeEffect(op->edgeId, effect, false, changedCallback);
    };

    switch (primitiveType)
    {
      case PrimitiveNodeTypes::PrimitiveType::Set:
        updateNode(static_cast<SetNode *>(node));
        break;
      case PrimitiveNodeTypes::PrimitiveType::List:
        updateNode(static_cast<ListNode *>(node));
        break;
      case PrimitiveNodeTypes::PrimitiveType::Map:
        updateNode(static_cast<MapNode *>(node));
        break;
      case PrimitiveNodeTypes::PrimitiveType::Reference:
        updateNode(static_cast<ReferenceNode *>(node));
        break;
      case PrimitiveNodeTypes::PrimitiveType::OrderedFloat64Map:
        updateNode(static_cast<OrderedFloat64MapNode *>(node));
        break;
      default:
        //not a container node
        break;
    }
  });
}

Promise<void> Core::applyUndoOperation(const Timestamp & ts, const Timestamp & prevTs,
  const UndoValueSetOperation * op, bool isUndo)
{
  return waitForNodeTypeReady(op->nodeId).then([this, prevTs, op, isUndo]()
  {
    NodeId nodeId = op->nodeId;
    Node * node = getNode(nodeId);
    int effect = (isUndo) ? -1 : 1;
    NodeType baseType = node->getBaseType();
    auto primitiveType = PrimitiveNodeTypes::nodeTypeToPrimitiveType(baseType);

    bool generateEvent = /* !(node->isVisible() */ true;

    auto updateNode = [&](auto * nodeType) {
      auto callback = [this, nodeId, generateEvent](auto newValue, auto oldValue) {
        Core::valueNodeChangedCallback<decltype(newValue)>(this, nodeId, generateEvent, newValue, oldValue);
      };
      nodeType->value.updateEffect(prevTs, effect, callback);
    };

    switch (primitiveType)
    {
      case PrimitiveNodeTypes::PrimitiveType::BoolValue:
        updateNode(static_cast<ValueNode<bool> *>(node));
        break;
      case PrimitiveNodeTypes::PrimitiveType::DoubleValue:
        updateNode(static_cast<ValueNode<double> *>(node));
        break;
      case PrimitiveNodeTypes::PrimitiveType::FloatValue:
        updateNode(static_cast<ValueNode<float> *>(node));
        break;
      case PrimitiveNodeTypes::PrimitiveType::Int32Value:
        updateNode(static_cast<ValueNode<int32_t> *>(node));
        break;
      case PrimitiveNodeTypes::PrimitiveType::Int64Value:
        updateNode(static_cast<ValueNode<int64_t> *>(node));
        break;
      case PrimitiveNodeTypes::PrimitiveType::Int8Value:
        updateNode(static_cast<ValueNode<int8_t> *>(node));
        break;
      default:
        //not a value node
        break;
    }
  });
}

Promise<void> Core::applyUndoOperation(const Timestamp & ts, const Timestamp & prevTs,
  const UndoBlockValueInsertAfterOperation * op, bool isUndo)
{
  return waitForNodeTypeReady(op->nodeId).then([this, ts, prevTs, op, isUndo]()
  {
    Node * node = getNode(op->nodeId);
    NodeType baseType = node->getBaseType();
    int effect = (isUndo) ? -1 : 1;

    auto callback = [this, ts, nodeId = op->nodeId](auto offset, auto data, auto length) {
      Core::blockValueNodeChangedCallback(this, ts, nodeId, offset, data, length);
    };

    if (baseType == PrimitiveNodeTypes::StringValue())
    {
      auto blockValueNode = static_cast<BlockValueNode<char> *>(node);
      blockValueNode->value.updateEffect(prevTs, 0, BlockData<char>::maxLength,
        effect, callback);
    }
    else
    {
      //not a block value node
    }
  });
}

Promise<void> Core::applyUndoOperation(const Timestamp & ts, const Timestamp & prevTs,
  const UndoBlockValueDeleteAfterOperation * op, bool isUndo)
{
  return waitForNodeTypeReady(op->nodeId).then([this, ts, op, isUndo]()
  {
    Node * node = getNode(op->nodeId);
    NodeType baseType = node->getBaseType();
    int effect = (isUndo) ? 1 : -1;

    auto callback = [this, ts, nodeId = op->nodeId](auto offset, auto data, auto length) {
      Core::blockValueNodeChangedCallback(this, ts, nodeId, offset, data, length);
    };

    if (baseType == PrimitiveNodeTypes::StringValue())
    {
      auto blockValueNode = static_cast<BlockValueNode<char> *>(node);
      blockValueNode->value.updateEffect(op->blockId, op->offset, op->length, effect, callback);
    }
    else
    {
      //not a block value node
    }
  });
}

void Core::unapplyOperation(const Timestamp & ts, const UndoGroupOperation * op)
{
  OperationIterator it(reinterpret_cast<const Operation *>(op->data), op->length);

  bool isUndo = op->type == OperationType::UndoGroupOperation;
  Timestamp childTs = ts;
  Timestamp childPrevTs = op->prevTs;

  while (*it)
  {
    unapplyUndoOperation(childTs, childPrevTs, *it, isUndo);
    ++it;
    ++childTs.clock;
    ++childPrevTs.clock;
  }
}

void Core::unapplyOperation(const Timestamp & ts, const GroupOperation * op)
{
  OperationIterator it(reinterpret_cast<const Operation *>(op->data), op->length);

  Timestamp childTs = ts;

  //NOTE: this is needed while the order of operations is important
  //  need to reverse ops when unapplying
  std::vector<std::pair<Timestamp, const Operation *>> operations;

  while (*it)
  {
    operations.push_back(std::make_pair(childTs, *it));
    ++it;
    ++childTs.clock;
  }

  std::reverse(operations.begin(), operations.end());

  for (auto i : operations)
  {
    unapplyOperation(i.first, i.second);
  }
}

void Core::unapplyOperation(const Timestamp & ts, const AtomicGroupOperation * op)
{
  OperationIterator it(reinterpret_cast<const Operation *>(op->data), op->length);

  while (*it)
  {
    unapplyOperation(ts, *it);
    ++it;
  }
}

void Core::unapplyOperation(const Timestamp & ts, const NodeCreateOperation * op)
{
  //this is a bit complicated
  //for now just do nothing
}

void Core::unapplyOperation(const Timestamp & ts, const EdgeCreateOperation * op)
{
  waitForNodeTypeReady(op->parentId).then([this, ts, op]()
  {
    Node * node = getNode(op->parentId);
    NodeType baseType = node->getBaseType();
    auto primitiveType = PrimitiveNodeTypes::nodeTypeToPrimitiveType(baseType);

    std::function<void(EdgeEvent &)> changedCallback = [this, op](EdgeEvent & event)
    {
      event.parentId = op->parentId;
      coreInit.eventRaised(event);
    };

    auto updateNode = [&](auto * nodeType) {
      nodeType->updateEdgeEffect(NodeId::inheritanceRootFor(ts), 0, true, changedCallback);
    };

    switch (primitiveType)
    {
      case PrimitiveNodeTypes::PrimitiveType::Set:
        updateNode(static_cast<SetNode *>(node));
        break;
      case PrimitiveNodeTypes::PrimitiveType::List:
        updateNode(static_cast<ListNode *>(node));
        break;
      case PrimitiveNodeTypes::PrimitiveType::Map:
        updateNode(static_cast<MapNode *>(node));
        break;
      case PrimitiveNodeTypes::PrimitiveType::Reference:
        updateNode(static_cast<ReferenceNode *>(node));
        break;
      case PrimitiveNodeTypes::PrimitiveType::OrderedFloat64Map:
        updateNode(static_cast<OrderedFloat64MapNode *>(node));
        break;
      default:
        //not a container node
        break;
    }
  });
}

void Core::unapplyOperation(const Timestamp & ts, const EdgeDeleteOperation * op)
{
  waitForNodeTypeReady(op->parentId).then([this, op]()
  {
    Node * node = getNode(op->parentId);
    NodeType baseType = node->getBaseType();
    auto primitiveType = PrimitiveNodeTypes::nodeTypeToPrimitiveType(baseType);

    std::function<void(EdgeEvent &)> changedCallback = [this, op](EdgeEvent & event)
    {
      event.parentId = op->parentId;
      coreInit.eventRaised(event);
    };

    int effect = 1;

    auto updateNode = [&](auto * nodeType) {
      nodeType->updateEdgeEffect(op->edgeId, effect, false, changedCallback);
    };

    switch (primitiveType)
    {
      case PrimitiveNodeTypes::PrimitiveType::Set:
        updateNode(static_cast<SetNode *>(node));
        break;
      case PrimitiveNodeTypes::PrimitiveType::List:
        updateNode(static_cast<ListNode *>(node));
        break;
      case PrimitiveNodeTypes::PrimitiveType::Map:
        updateNode(static_cast<MapNode *>(node));
        break;
      case PrimitiveNodeTypes::PrimitiveType::Reference:
        updateNode(static_cast<ReferenceNode *>(node));
        break;
      case PrimitiveNodeTypes::PrimitiveType::OrderedFloat64Map:
        updateNode(static_cast<OrderedFloat64MapNode *>(node));
        break;
      default:
        //not a container node
        break;
    }
  });
}

void Core::unapplyOperation(const Timestamp & ts, const ValueSetOperation * op)
{
  waitForNodeTypeReady(op->nodeId).then([this, ts, op]()
  {
    NodeId nodeId = op->nodeId;
    Node * node = getNode(nodeId);
    NodeType baseType = node->getBaseType();
    auto primitiveType = PrimitiveNodeTypes::nodeTypeToPrimitiveType(baseType);

    bool generateEvent = /* !(node->isVisible() */ true;

    auto updateNode = [&](auto * nodeType) {
      auto callback = [this, nodeId, generateEvent](auto newValue, auto oldValue) {
        Core::valueNodeChangedCallback<decltype(newValue)>(this, nodeId, generateEvent, newValue, oldValue);
      };
      auto valueNode = static_cast<decltype(nodeType)>(node);
      valueNode->value.deinitializeValue(ts, callback);
    };

    switch (primitiveType)
    {
      case PrimitiveNodeTypes::PrimitiveType::BoolValue:
        updateNode(static_cast<ValueNode<bool> *>(node));
        break;
      case PrimitiveNodeTypes::PrimitiveType::DoubleValue:
        updateNode(static_cast<ValueNode<double> *>(node));
        break;
      case PrimitiveNodeTypes::PrimitiveType::FloatValue:
        updateNode(static_cast<ValueNode<float> *>(node));
        break;
      case PrimitiveNodeTypes::PrimitiveType::Int32Value:
        updateNode(static_cast<ValueNode<int32_t> *>(node));
        break;
      case PrimitiveNodeTypes::PrimitiveType::Int64Value:
        updateNode(static_cast<ValueNode<int64_t> *>(node));
        break;
      case PrimitiveNodeTypes::PrimitiveType::Int8Value:
        updateNode(static_cast<ValueNode<int8_t> *>(node));
        break;
      default:
        //not a value node
        break;
    }
  });
}

void Core::unapplyOperation(const Timestamp & ts, const BlockValueInsertAfterOperation * op)
{
  waitForNodeTypeReady(op->nodeId).then([this, ts, op]()
  {
    Node * node = getNode(op->nodeId);
    NodeType baseType = node->getBaseType();

    auto callback = [this, ts, nodeId = op->nodeId](auto offset, auto data, auto length) {
      Core::blockValueNodeChangedCallback(this, ts, nodeId, offset, data, length);
    };

    if (baseType == PrimitiveNodeTypes::StringValue())
    {
      auto blockValueNode = static_cast<BlockValueNode<char> *>(node);
      blockValueNode->value.deinitializeBlock(ts, callback);
    }
    else
    {
      //not a block value node
    }
  });
}

void Core::unapplyOperation(const Timestamp & ts, const BlockValueDeleteAfterOperation * op)
{
  waitForNodeTypeReady(op->nodeId).then([this, ts, op]()
  {
    Node * node = getNode(op->nodeId);
    NodeType baseType = node->getBaseType();
    int effect = 1;

    auto callback = [this, ts, nodeId = op->nodeId](auto offset, auto data, auto length) {
      Core::blockValueNodeChangedCallback(this, ts, nodeId, offset, data, length);
    };

    if (baseType == PrimitiveNodeTypes::StringValue())
    {
      auto blockValueNode = static_cast<BlockValueNode<char> *>(node);
      blockValueNode->value.updateEffect(op->blockId, op->offset, op->length, effect, callback);
    }
    else
    {
      //not a block value node
    }
  });
}

void Core::unapplyUndoOperation(const Timestamp & ts, const Timestamp & prevTs, const UndoEdgeCreateOperation * op, bool isUndo)
{
  waitForNodeTypeReady(op->parentId).then([this, prevTs, op, isUndo]()
  {
    Node * node = getNode(op->parentId);
    NodeType baseType = node->getBaseType();
    auto primitiveType = PrimitiveNodeTypes::nodeTypeToPrimitiveType(baseType);

    std::function<void(EdgeEvent &)> changedCallback = [this, op](EdgeEvent & event)
    {
      event.parentId = op->parentId;
      coreInit.eventRaised(event);
    };

    int effect = (isUndo) ? 1 : -1;

    auto updateNode = [&](auto * nodeType) {
      nodeType->updateEdgeEffect(NodeId::inheritanceRootFor(prevTs), effect, false, changedCallback);
    };

    switch (primitiveType)
    {
      case PrimitiveNodeTypes::PrimitiveType::Set:
        updateNode(static_cast<SetNode *>(node));
        break;
      case PrimitiveNodeTypes::PrimitiveType::List:
        updateNode(static_cast<ListNode *>(node));
        break;
      case PrimitiveNodeTypes::PrimitiveType::Map:
        updateNode(static_cast<MapNode *>(node));
        break;
      case PrimitiveNodeTypes::PrimitiveType::Reference:
        updateNode(static_cast<ReferenceNode *>(node));
        break;
      case PrimitiveNodeTypes::PrimitiveType::OrderedFloat64Map:
        updateNode(static_cast<OrderedFloat64MapNode *>(node));
        break;
      default:
        //not a container node
        break;
    }
  });
}

void Core::unapplyUndoOperation(const Timestamp & ts, const Timestamp & prevTs, const UndoEdgeDeleteOperation * op, bool isUndo)
{
  waitForNodeTypeReady(op->parentId).then([this, op, isUndo]()
  {
    Node * node = getNode(op->parentId);
    NodeType baseType = node->getBaseType();
    auto primitiveType = PrimitiveNodeTypes::nodeTypeToPrimitiveType(baseType);

    std::function<void(EdgeEvent &)> changedCallback = [this, op](EdgeEvent & event)
    {
      event.parentId = op->parentId;
      coreInit.eventRaised(event);
    };

    int effect = (isUndo) ? -1 : 1;

    auto updateNode = [&](auto * nodeType) {
      nodeType->updateEdgeEffect(op->edgeId, effect, false, changedCallback);
    };

    switch (primitiveType)
    {
      case PrimitiveNodeTypes::PrimitiveType::Set:
        updateNode(static_cast<SetNode *>(node));
        break;
      case PrimitiveNodeTypes::PrimitiveType::List:
        updateNode(static_cast<ListNode *>(node));
        break;
      case PrimitiveNodeTypes::PrimitiveType::Map:
        updateNode(static_cast<MapNode *>(node));
        break;
      case PrimitiveNodeTypes::PrimitiveType::Reference:
        updateNode(static_cast<ReferenceNode *>(node));
        break;
      case PrimitiveNodeTypes::PrimitiveType::OrderedFloat64Map:
        updateNode(static_cast<OrderedFloat64MapNode *>(node));
        break;
      default:
        //not a container node
        break;
    }
  });
}

void Core::unapplyUndoOperation(const Timestamp & ts, const Timestamp & prevTs, const UndoValueSetOperation * op, bool isUndo)
{
  waitForNodeTypeReady(op->nodeId).then([this, prevTs, op, isUndo]()
  {
    NodeId nodeId = op->nodeId;
    Node * node = getNode(nodeId);
    int effect = (isUndo) ? 1 : -1;
    NodeType baseType = node->getBaseType();
    auto primitiveType = PrimitiveNodeTypes::nodeTypeToPrimitiveType(baseType);

    bool generateEvent = /* !(node->isVisible() */ true;

    auto updateNode = [&](auto * nodeType) {
      auto callback = [this, nodeId, generateEvent](auto newValue, auto oldValue) {
        Core::valueNodeChangedCallback<decltype(newValue)>(this, nodeId, generateEvent, newValue, oldValue);
      };
      auto valueNode = static_cast<decltype(nodeType)>(node);
      valueNode->value.updateEffect(prevTs, effect, callback);
    };

    switch (primitiveType)
    {
      case PrimitiveNodeTypes::PrimitiveType::BoolValue:
        updateNode(static_cast<ValueNode<bool> *>(node));
        break;
      case PrimitiveNodeTypes::PrimitiveType::DoubleValue:
        updateNode(static_cast<ValueNode<double> *>(node));
        break;
      case PrimitiveNodeTypes::PrimitiveType::FloatValue:
        updateNode(static_cast<ValueNode<float> *>(node));
        break;
      case PrimitiveNodeTypes::PrimitiveType::Int32Value:
        updateNode(static_cast<ValueNode<int32_t> *>(node));
        break;
      case PrimitiveNodeTypes::PrimitiveType::Int64Value:
        updateNode(static_cast<ValueNode<int64_t> *>(node));
        break;
      case PrimitiveNodeTypes::PrimitiveType::Int8Value:
        updateNode(static_cast<ValueNode<int8_t> *>(node));
        break;
      default:
        //not a value node
        break;
    }
  });
}

void Core::unapplyUndoOperation(const Timestamp & ts, const Timestamp & prevTs, const UndoBlockValueInsertAfterOperation * op, bool isUndo)
{
  waitForNodeTypeReady(op->nodeId).then([this, ts, prevTs, op, isUndo]()
  {
    Node * node = getNode(op->nodeId);
    NodeType baseType = node->getBaseType();
    int effect = (isUndo) ? 1 : -1;

    auto callback = [this, ts, nodeId = op->nodeId](auto offset, auto data, auto length) {
      Core::blockValueNodeChangedCallback(this, ts, nodeId, offset, data, length);
    };

    if (baseType == PrimitiveNodeTypes::StringValue())
    {
      auto blockValueNode = static_cast<BlockValueNode<char> *>(node);
      blockValueNode->value.updateEffect(prevTs, 0, BlockData<char>::maxLength, effect, callback);
    }
    else
    {
      //not a block value node
    }
  });
}

void Core::unapplyUndoOperation(const Timestamp & ts, const Timestamp & prevTs, const UndoBlockValueDeleteAfterOperation * op, bool isUndo)
{
  waitForNodeTypeReady(op->nodeId).then([this, ts, op, isUndo]()
  {
    Node * node = getNode(op->nodeId);
    NodeType baseType = node->getBaseType();
    int effect = (isUndo) ? -1 : 1;

    auto callback = [this, ts, nodeId = op->nodeId](auto offset, auto data, auto length) {
      Core::blockValueNodeChangedCallback(this, ts, nodeId, offset, data, length);
    };

    if (baseType == PrimitiveNodeTypes::StringValue())
    {
      auto blockValueNode = static_cast<BlockValueNode<char> *>(node);
      blockValueNode->value.updateEffect(op->blockId, op->offset, op->length, effect, callback);
    }
    else
    {
      //not a block value node
    }
  });
}

void Core::serializeNode(IObjectSerializer & serializer, const NodeId & nodeId) const
{
  const Node * node = getExistingNode(nodeId);

  if (node == nullptr)
  {
    serializer.addNullValue();
    return;
  }

  NodeType type = node->getBaseType();
  auto primitiveType = PrimitiveNodeTypes::nodeTypeToPrimitiveType(type);

  switch (primitiveType)
  {
    case PrimitiveNodeTypes::PrimitiveType::StringValue:
      static_cast<const BlockValueNode<char> *>(node)->serialize(serializer);
      break;
    case PrimitiveNodeTypes::PrimitiveType::BoolValue:
      static_cast<const ValueNode<bool> *>(node)->serialize(serializer);
      break;
    case PrimitiveNodeTypes::PrimitiveType::DoubleValue:
      static_cast<const ValueNode<double> *>(node)->serialize(serializer);
      break;
    case PrimitiveNodeTypes::PrimitiveType::FloatValue:
      static_cast<const ValueNode<float> *>(node)->serialize(serializer);
      break;
    case PrimitiveNodeTypes::PrimitiveType::Int32Value:
      static_cast<const ValueNode<int32_t> *>(node)->serialize(serializer);
      break;
    case PrimitiveNodeTypes::PrimitiveType::Int64Value:
      static_cast<const ValueNode<int64_t> *>(node)->serialize(serializer);
      break;
    case PrimitiveNodeTypes::PrimitiveType::Int8Value:
      static_cast<const ValueNode<int8_t> *>(node)->serialize(serializer);
      break;
    case PrimitiveNodeTypes::PrimitiveType::Set:
      static_cast<const SetNode *>(node)->serialize(serializer);
      break;
    case PrimitiveNodeTypes::PrimitiveType::List:
      static_cast<const ListNode *>(node)->serialize(serializer);
      break;
    case PrimitiveNodeTypes::PrimitiveType::Map:
      static_cast<const MapNode *>(node)->serialize(serializer);
      break;
    case PrimitiveNodeTypes::PrimitiveType::Reference:
      static_cast<const ReferenceNode *>(node)->serialize(serializer);
      break;
    case PrimitiveNodeTypes::PrimitiveType::OrderedFloat64Map:
      static_cast<const OrderedFloat64MapNode *>(node)->serialize(serializer);
      break;
    default:
      node->serialize(serializer);
      break;
  }
}

void Core::serializeNodeChildren(IObjectSerializer & serializer, const NodeId & nodeId, bool includePending) const
{
  const Node * node = getExistingNode(nodeId);

  if (node == nullptr)
  {
    serializer.addNullValue();
    return;
  }

  NodeType type = node->getBaseType();
  auto primitiveType = PrimitiveNodeTypes::nodeTypeToPrimitiveType(type);

  switch (primitiveType)
  {
    case PrimitiveNodeTypes::PrimitiveType::Set:
      static_cast<const SetNode *>(node)->serializeChildren(serializer, includePending);
      break;
    case PrimitiveNodeTypes::PrimitiveType::List:
      static_cast<const ListNode *>(node)->serializeChildren(serializer, includePending);
      break;
    case PrimitiveNodeTypes::PrimitiveType::Map:
      static_cast<const MapNode *>(node)->serializeChildren(serializer, includePending);
      break;
    case PrimitiveNodeTypes::PrimitiveType::Reference:
      static_cast<const ReferenceNode *>(node)->serializeChildren(serializer, includePending);
      break;
    case PrimitiveNodeTypes::PrimitiveType::OrderedFloat64Map:
      static_cast<const OrderedFloat64MapNode *>(node)->serializeChildren(serializer, includePending);
      break;
    default:
      serializer.addNullValue();
      break;
  }
}

double Core::getNodeValue(const NodeId & nodeId) const
{
  const Node * node = getExistingNode(nodeId);

  if (node == nullptr)
  {
    return 0;
  }

  NodeType type = node->getBaseType();
  auto primitiveType = PrimitiveNodeTypes::nodeTypeToPrimitiveType(type);

  switch (primitiveType)
  {
    case PrimitiveNodeTypes::PrimitiveType::BoolValue:
    {
      auto * valueNode = static_cast<const ValueNode<bool> *>(node);
      return static_cast<double>(valueNode->value.getValue());
      break;
    }
    case PrimitiveNodeTypes::PrimitiveType::DoubleValue:
    {
      auto * valueNode = static_cast<const ValueNode<double> *>(node);
      return static_cast<double>(valueNode->value.getValue());
      break;
    }
    case PrimitiveNodeTypes::PrimitiveType::FloatValue:
    {
      auto * valueNode = static_cast<const ValueNode<float> *>(node);
      return static_cast<double>(valueNode->value.getValue());
      break;
    }
    case PrimitiveNodeTypes::PrimitiveType::Int32Value:
    {
      auto * valueNode = static_cast<const ValueNode<int32_t> *>(node);
      return static_cast<double>(valueNode->value.getValue());
      break;
    }
    case PrimitiveNodeTypes::PrimitiveType::Int64Value:
    {
      auto * valueNode = static_cast<const ValueNode<int64_t> *>(node);
      return static_cast<double>(valueNode->value.getValue());
      break;
    }
    case PrimitiveNodeTypes::PrimitiveType::Int8Value:
    {
      auto * valueNode = static_cast<const ValueNode<int8_t> *>(node);
      return static_cast<double>(valueNode->value.getValue());
      break;
    }
    default:
      return 0;
      break;
  }
}

void Core::getNodeBlockValue(std::string & outString, const NodeId & nodeId) const
{
  const Node * node = getExistingNode(nodeId);

  if (node == nullptr)
  {
    return;
  }

  NodeType type = node->getBaseType();

  if (type == PrimitiveNodeTypes::StringValue())
  {
    auto * blockValueNode = static_cast<const BlockValueNode<char> *>(node);
    outString += blockValueNode->value.toString();
  }
}