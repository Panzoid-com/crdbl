#include "Operation.h"
#include "OperationIterator.h" //temp, for transform timestamps
#include <cstring>

size_t Operation::getStructSize(OperationType type)
{
  switch(type)
  {
    case OperationType::NoOpOperation:
    {
      return sizeof(NoOpOperation);
    }
    case OperationType::UndoGroupOperation:
    case OperationType::RedoGroupOperation:
    {
      return sizeof(UndoGroupOperation);
    }
    case OperationType::GroupOperation:
    case OperationType::AtomicGroupOperation:
    {
      return sizeof(GroupOperation);
    }
    case OperationType::SetAttributeOperation:
    {
      return sizeof(SetAttributeOperation);
    }
    case OperationType::NodeCreateOperation:
    {
      return sizeof(NodeCreateOperation);
    }
    case OperationType::EdgeCreateOperation:
    {
      return sizeof(EdgeCreateOperation);
    }
    case OperationType::UndoEdgeCreateOperation:
    {
      return sizeof(UndoEdgeCreateOperation);
    }
    case OperationType::EdgeDeleteOperation:
    {
      return sizeof(EdgeDeleteOperation);
    }
    case OperationType::UndoEdgeDeleteOperation:
    {
      return sizeof(UndoEdgeDeleteOperation);
    }
    case OperationType::ValuePreviewOperation:
    case OperationType::ValueSetOperation:
    {
      return sizeof(ValueSetOperation);
    }
    case OperationType::UndoValueSetOperation:
    {
      return sizeof(UndoValueSetOperation);
    }
    case OperationType::BlockValueInsertAfterOperation:
    {
      return sizeof(BlockValueInsertAfterOperation);
    }
    case OperationType::UndoBlockValueInsertAfterOperation:
    {
      return sizeof(UndoBlockValueInsertAfterOperation);
    }
    case OperationType::BlockValueDeleteAfterOperation:
    {
      return sizeof(BlockValueDeleteAfterOperation);
    }
    case OperationType::UndoBlockValueDeleteAfterOperation:
    {
      return sizeof(UndoBlockValueDeleteAfterOperation);
    }
    default:
      return sizeof(Operation);
  }
}

size_t Operation::getSize(OperationType type, size_t dataSize)
{
  size_t size = Operation::getStructSize(type);

  switch(type)
  {
    case OperationType::UndoGroupOperation:
    case OperationType::RedoGroupOperation:
    case OperationType::GroupOperation:
    case OperationType::AtomicGroupOperation:
    case OperationType::SetAttributeOperation:
    case OperationType::NodeCreateOperation:
    case OperationType::ValuePreviewOperation:
    case OperationType::ValueSetOperation:
    case OperationType::BlockValueInsertAfterOperation:
    {
      size += dataSize;
      break;
    }
    default:
      break;
  }

  return size;
}

size_t Operation::getStructSize() const
{
  return Operation::getStructSize(type);
};

size_t Operation::getDataSize() const
{
  switch(type)
  {
    case OperationType::UndoGroupOperation:
    case OperationType::RedoGroupOperation:
    {
      auto op = reinterpret_cast<const UndoGroupOperation *>(this);
      return op->length;
    }
    case OperationType::GroupOperation:
    case OperationType::AtomicGroupOperation:
    {
      auto op = reinterpret_cast<const GroupOperation *>(this);
      return op->length;
    }
    case OperationType::SetAttributeOperation:
    {
      auto op = reinterpret_cast<const SetAttributeOperation *>(this);
      return op->length;
    }
    case OperationType::NodeCreateOperation:
    {
      auto op = reinterpret_cast<const NodeCreateOperation *>(this);
      return op->nodeTypeLength;
    }
    case OperationType::ValuePreviewOperation:
    case OperationType::ValueSetOperation:
    {
      auto op = reinterpret_cast<const ValueSetOperation *>(this);
      return op->length;
    }
    case OperationType::BlockValueInsertAfterOperation:
    {
      auto op = reinterpret_cast<const BlockValueInsertAfterOperation *>(this);
      return op->length;
    }
    default:
      return 0;
  }
}

size_t Operation::getSize() const
{
  switch(type)
  {
    case OperationType::UndoGroupOperation:
    case OperationType::RedoGroupOperation:
    {
      auto op = reinterpret_cast<const UndoGroupOperation *>(this);
      return sizeof(*op) + op->length;
    }
    case OperationType::GroupOperation:
    case OperationType::AtomicGroupOperation:
    {
      auto op = reinterpret_cast<const GroupOperation *>(this);
      return sizeof(*op) + op->length;
    }
    case OperationType::SetAttributeOperation:
    {
      auto op = reinterpret_cast<const SetAttributeOperation *>(this);
      return sizeof(*op) + op->length;
    }
    case OperationType::NodeCreateOperation:
    {
      auto op = reinterpret_cast<const NodeCreateOperation *>(this);
      return sizeof(*op) + op->nodeTypeLength;
    }
    case OperationType::ValuePreviewOperation:
    case OperationType::ValueSetOperation:
    {
      auto op = reinterpret_cast<const ValueSetOperation *>(this);
      return sizeof(*op) + op->length;
    }
    case OperationType::BlockValueInsertAfterOperation:
    {
      auto op = reinterpret_cast<const BlockValueInsertAfterOperation *>(this);
      return sizeof(*op) + op->length;
    }
    default:
      return getStructSize();
  }
}

void Operation::transformTimestamps(const Timestamp & offset)
{
  OperationType opType = type;

  switch(opType)
  {
    case OperationType::AtomicGroupOperation:
    {
      auto op = reinterpret_cast<AtomicGroupOperation *>(this);
      OperationIterator it(reinterpret_cast<Operation *>(op->data), op->length);
      while (*it)
      {
        const_cast<Operation *>(*it)->transformTimestamps(offset);
        ++it;
      }
      break;
    }
    case OperationType::SetAttributeOperation:
    {
      //NOTE: bit of a hack
      auto op = reinterpret_cast<SetAttributeOperation *>(this);
      if (op->length == sizeof(EdgeId))
      {
        EdgeId * edge = reinterpret_cast<EdgeId *>(op->data);
        if (edge->ts.site == 0)
        {
          if (edge->ts != Timestamp::Null)
          {
            edge->ts += offset;
          }
        }
      }
      break;
    }
    case OperationType::EdgeCreateOperation:
    {
      auto op = reinterpret_cast<EdgeCreateOperation *>(this);
      op->parentId.ts += offset;
      op->childId.ts += offset;
      break;
    }
    case OperationType::EdgeDeleteOperation:
    {
      auto op = reinterpret_cast<EdgeDeleteOperation *>(this);
      op->parentId.ts += offset;
      op->edgeId.ts += offset;
      break;
    }
    case OperationType::ValueSetOperation:
    {
      auto op = reinterpret_cast<ValueSetOperation *>(this);
      op->nodeId.ts += offset;
      break;
    }
    case OperationType::BlockValueInsertAfterOperation:
    {
      auto op = reinterpret_cast<BlockValueInsertAfterOperation *>(this);
      op->nodeId.ts += offset;
      if (op->blockId != Timestamp::Null)
      {
        op->blockId += offset;
      }
      break;
    }
    case OperationType::BlockValueDeleteAfterOperation:
    {
      auto op = reinterpret_cast<BlockValueDeleteAfterOperation *>(this);
      op->nodeId.ts += offset;
      if (op->blockId != Timestamp::Null)
      {
        op->blockId += offset;
      }
      break;
    }
    default:
      break;
  }
}

bool Operation::isUndo() const
{
  return type == OperationType::UndoGroupOperation;
}

bool Operation::isRedo() const
{
  return type == OperationType::RedoGroupOperation;
}

bool Operation::isGroup() const
{
  return type == OperationType::AtomicGroupOperation ||
    type == OperationType::GroupOperation ||
    type == OperationType::UndoGroupOperation ||
    type == OperationType::RedoGroupOperation;
}

uint32_t Operation::getMaxTsOffset() const
{
  switch (type)
  {
    case OperationType::GroupOperation:
    {
      uint32_t offset = 0;
      const GroupOperation * op = reinterpret_cast<const GroupOperation *>(this);
      OperationIterator it(reinterpret_cast<const Operation *>(op->data), op->length);
      while (*it)
      {
        ++offset;
        ++it;
      }
      return offset;
    }
    case OperationType::UndoGroupOperation:
    case OperationType::RedoGroupOperation:
    {
      uint32_t offset = 0;
      const UndoGroupOperation * op = reinterpret_cast<const UndoGroupOperation *>(this);
      OperationIterator it(reinterpret_cast<const Operation *>(op->data), op->length);
      while (*it)
      {
        ++offset;
        ++it;
      }
      return offset;
    }
    default:
      return 0;
  }
}

bool Operation::operator==(const Operation & rhs) const
{
  if (type != rhs.type)
  {
    return false;
  }

  //NOTE: this is a bit of a lazy implementation and might not handle some
  //  cases where junk values in struct padding cause incorrect inequality

  size_t size = getSize();
  if (size != rhs.getSize())
  {
    return false;
  }

  return !memcmp(reinterpret_cast<const char *>(this),
    reinterpret_cast<const char *>(&rhs), size);
}