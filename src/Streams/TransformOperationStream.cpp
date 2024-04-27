#include "TransformOperationStream.h"
#include <stdexcept>

bool TransformOperationStream::write(const RefCounted<const LogOperation> & data)
{
  size_t size = data->getSize();
  auto clone = reinterpret_cast<LogOperation *>(new uint8_t[size]);
  memcpy(clone, &(*data), size);

  clone->ts = transformTimestamp(clone->ts);
  transformOperation(&clone->op);

  writeToDestination(RefCounted<const LogOperation>(clone));

  // writeToDestination(data);

  return true;
}

void TransformOperationStream::transformOperation(Operation * operation)
{
  OperationType opType = operation->type;

  switch(opType)
  {
    case OperationType::GroupOperation:
    case OperationType::AtomicGroupOperation:
    {
      auto op = reinterpret_cast<GroupOperation *>(operation);
      OperationIterator it(reinterpret_cast<Operation *>(op->data), op->length);
      while (*it)
      {
        transformOperation(const_cast<Operation *>(*it));
        ++it;
      }
      break;
    }
    case OperationType::UndoGroupOperation:
    case OperationType::RedoGroupOperation:
    {
      auto op = reinterpret_cast<UndoGroupOperation *>(operation);

      op->prevTs = transformTimestamp(op->prevTs);

      OperationIterator it(reinterpret_cast<Operation *>(op->data), op->length);
      while (*it)
      {
        transformOperation(const_cast<Operation *>(*it));
        ++it;
      }

      break;
    }
    case OperationType::NodeCreateOperation:
    {
      auto op = reinterpret_cast<NodeCreateOperation *>(operation);

      std::string nodeType(reinterpret_cast<const char *>(op->data), op->nodeTypeLength);
      std::string newNodeType = transformNodeType(nodeType);

      if (newNodeType != nodeType)
      {
        if (newNodeType.size() == 0)
        {
          //TODO: insert NoOp instead of create
          throw std::runtime_error("NoOp not yet implemented");
        }
        else if (newNodeType.size() != op->nodeTypeLength)
        {
          //since node ids are just being updated in place, they are all assumed
          //  to be the same length, which is essentially true for practical use
          throw std::runtime_error("Node type length mismatch");
        }
        memcpy(op->data, newNodeType.data(), op->nodeTypeLength);
      }

      break;
    }
    case OperationType::SetAttributeOperation:
    {
      //NOTE: a bit of a hack
      auto op = reinterpret_cast<SetAttributeOperation *>(operation);
      if (op->length == sizeof(EdgeId))
      {
        EdgeId * edge = reinterpret_cast<EdgeId *>(op->data);

        if (edge->ts.site == 0)
        {
          if (edge->ts != Timestamp::Null)
          {
            *edge = transformEdgeId(*edge);
          }
        }
      }
      break;
    }
    case OperationType::EdgeCreateOperation:
    {
      auto op = reinterpret_cast<EdgeCreateOperation *>(operation);
      op->parentId = transformNodeId(op->parentId);
      op->childId = transformNodeId(op->childId);
      break;
    }
    case OperationType::UndoEdgeCreateOperation:
    case OperationType::RedoEdgeCreateOperation:
    {
      auto op = reinterpret_cast<UndoEdgeCreateOperation *>(operation);
      op->parentId = transformNodeId(op->parentId);
      break;
    }
    case OperationType::EdgeDeleteOperation:
    {
      auto op = reinterpret_cast<EdgeDeleteOperation *>(operation);
      op->parentId = transformNodeId(op->parentId);
      op->edgeId = transformEdgeId(op->edgeId);
      break;
    }
    case OperationType::UndoEdgeDeleteOperation:
    case OperationType::RedoEdgeDeleteOperation:
    {
      auto op = reinterpret_cast<UndoEdgeDeleteOperation *>(operation);
      op->parentId = transformNodeId(op->parentId);
      op->edgeId = transformEdgeId(op->edgeId);
      break;
    }
    case OperationType::ValueSetOperation:
    {
      auto op = reinterpret_cast<ValueSetOperation *>(operation);
      op->nodeId = transformNodeId(op->nodeId);
      break;
    }
    case OperationType::UndoValueSetOperation:
    case OperationType::RedoValueSetOperation:
    {
      auto op = reinterpret_cast<UndoValueSetOperation *>(operation);
      op->nodeId = transformNodeId(op->nodeId);
      break;
    }
    case OperationType::BlockValueInsertAfterOperation:
    {
      auto op = reinterpret_cast<BlockValueInsertAfterOperation *>(operation);
      op->nodeId = transformNodeId(op->nodeId);
      op->blockId = transformTimestamp(op->blockId);
      break;
    }
    case OperationType::UndoBlockValueInsertAfterOperation:
    case OperationType::RedoBlockValueInsertAfterOperation:
    {
      auto op = reinterpret_cast<UndoBlockValueInsertAfterOperation *>(operation);
      op->nodeId = transformNodeId(op->nodeId);
      break;
    }
    case OperationType::BlockValueDeleteAfterOperation:
    {
      auto op = reinterpret_cast<BlockValueDeleteAfterOperation *>(operation);
      op->nodeId = transformNodeId(op->nodeId);
      op->blockId = transformTimestamp(op->blockId);
      break;
    }
    case OperationType::UndoBlockValueDeleteAfterOperation:
    case OperationType::RedoBlockValueDeleteAfterOperation:
    {
      auto op = reinterpret_cast<UndoBlockValueDeleteAfterOperation *>(operation);
      op->nodeId = transformNodeId(op->nodeId);
      op->blockId = transformTimestamp(op->blockId);
      break;
    }
    default:
      break;
  }
}

NodeId TransformOperationStream::transformNodeId(const NodeId & nodeId)
{
  int32_t offset = 0;

  if (nodeId.child > 0)
  {
    auto rootNode = core->getExistingNode({ nodeId.ts, 0 });
    if (rootNode == nullptr)
    {
      //is this check even needed? this would be quite unusual if even possible
      return nodeId;
    }

    auto it = typeOffsetMap.find(rootNode->getType().toString());
    if (it != typeOffsetMap.end())
    {
      auto & offsetMap = it->second;
      auto offsetIt = offsetMap.find(nodeId.child);

      if (offsetIt != offsetMap.end())
      {
        offset = offsetIt->second;
      }
    }
  }

  return { transformTimestamp(nodeId.ts), nodeId.child + offset };
}

EdgeId TransformOperationStream::transformEdgeId(const EdgeId & edgeId)
{
  int32_t offset = 0;

  if (edgeId.child > 0)
  {
    auto rootNode = core->getExistingNode({ edgeId.ts, 0 });
    if (rootNode == nullptr)
    {
      //is this check even needed? this would be quite unusual if even possible
      return edgeId;
    }

    auto it = typeEdgeOffsetMap.find(rootNode->getType().toString());
    if (it != typeEdgeOffsetMap.end())
    {
      auto & offsetMap = it->second;
      auto offsetIt = offsetMap.find(edgeId.child);

      if (offsetIt != offsetMap.end())
      {
        offset = offsetIt->second;
      }
    }
  }

  return { transformTimestamp(edgeId.ts), edgeId.child + offset };
}

Timestamp TransformOperationStream::transformTimestamp(const Timestamp & timestamp)
{
  // if (local && timestamp.site > 0)
  // {
  //   return { timestamp.clock, 1 };
  // }

  return timestamp;
}

std::string TransformOperationStream::transformNodeType(std::string nodeType)
{
  auto it = typeMap.find(nodeType);
  if (it != typeMap.end())
  {
    return it->second;
  }

  return nodeType;
}

void TransformOperationStream::close()
{

}

void TransformOperationStream::mapType(std::string fromTypeId, std::string toTypeId)
{
  typeMap[fromTypeId] = toTypeId;
}

void TransformOperationStream::mapTypeNodeId(std::string typeId, uint32_t fromOffset, uint32_t toOffset)
{
  auto & offsetMap = typeOffsetMap[typeId];
  offsetMap[fromOffset] = (int32_t)toOffset - (int32_t)fromOffset;
}

void TransformOperationStream::mapTypeEdgeId(std::string typeId, uint32_t fromOffset, uint32_t toOffset)
{
  auto & offsetMap = typeEdgeOffsetMap[typeId];
  offsetMap[fromOffset] = (int32_t)toOffset - (int32_t)fromOffset;
}