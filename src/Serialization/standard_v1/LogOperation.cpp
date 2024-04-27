#include "LogOperation.h"

namespace Serialization_standard_v1
{
  size_t Operation::getStructSize(OperationType type)
  {
    switch(static_cast<::OperationType>(type))
    {
      case ::OperationType::NoOpOperation:
      {
        return sizeof(NoOpOperation);
      }
      case ::OperationType::UndoGroupOperation:
      case ::OperationType::RedoGroupOperation:
      {
        return sizeof(UndoGroupOperation);
      }
      case ::OperationType::GroupOperation:
      case ::OperationType::AtomicGroupOperation:
      {
        return sizeof(GroupOperation);
      }
      case ::OperationType::SetAttributeOperation:
      {
        return sizeof(SetAttributeOperation);
      }
      case ::OperationType::NodeCreateOperation:
      {
        return sizeof(NodeCreateOperation);
      }
      case ::OperationType::EdgeCreateOperation:
      {
        return sizeof(EdgeCreateOperation);
      }
      case ::OperationType::UndoEdgeCreateOperation:
      {
        return sizeof(UndoEdgeCreateOperation);
      }
      case ::OperationType::EdgeDeleteOperation:
      {
        return sizeof(EdgeDeleteOperation);
      }
      case ::OperationType::UndoEdgeDeleteOperation:
      {
        return sizeof(UndoEdgeDeleteOperation);
      }
      case ::OperationType::ValuePreviewOperation:
      case ::OperationType::ValueSetOperation:
      {
        return sizeof(ValueSetOperation);
      }
      case ::OperationType::UndoValueSetOperation:
      {
        return sizeof(UndoValueSetOperation);
      }
      case ::OperationType::BlockValueInsertAfterOperation:
      {
        return sizeof(BlockValueInsertAfterOperation);
      }
      case ::OperationType::UndoBlockValueInsertAfterOperation:
      {
        return sizeof(UndoBlockValueInsertAfterOperation);
      }
      case ::OperationType::BlockValueDeleteAfterOperation:
      {
        return sizeof(BlockValueDeleteAfterOperation);
      }
      case ::OperationType::UndoBlockValueDeleteAfterOperation:
      {
        return sizeof(UndoBlockValueDeleteAfterOperation);
      }
      default:
        return 0;
    }
  }

  size_t Operation::getSize(OperationType type, size_t dataSize)
  {
    size_t size = Operation::getStructSize(type);

    switch(static_cast<::OperationType>(type))
    {
      case ::OperationType::UndoGroupOperation:
      case ::OperationType::RedoGroupOperation:
      case ::OperationType::GroupOperation:
      case ::OperationType::AtomicGroupOperation:
      case ::OperationType::SetAttributeOperation:
      case ::OperationType::NodeCreateOperation:
      case ::OperationType::ValuePreviewOperation:
      case ::OperationType::ValueSetOperation:
      case ::OperationType::BlockValueInsertAfterOperation:
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
  }

  size_t Operation::getDataSize() const
  {
    switch(static_cast<::OperationType>(type))
    {
      case ::OperationType::UndoGroupOperation:
      case ::OperationType::RedoGroupOperation:
      {
        auto op = reinterpret_cast<const UndoGroupOperation *>(this);
        return op->length;
      }
      case ::OperationType::GroupOperation:
      case ::OperationType::AtomicGroupOperation:
      {
        auto op = reinterpret_cast<const GroupOperation *>(this);
        return op->length;
      }
      case ::OperationType::SetAttributeOperation:
      {
        auto op = reinterpret_cast<const SetAttributeOperation *>(this);
        return op->length;
      }
      case ::OperationType::NodeCreateOperation:
      {
        auto op = reinterpret_cast<const NodeCreateOperation *>(this);
        return op->nodeTypeLength;
      }
      case ::OperationType::ValuePreviewOperation:
      case ::OperationType::ValueSetOperation:
      {
        auto op = reinterpret_cast<const ValueSetOperation *>(this);
        return op->length;
      }
      case ::OperationType::BlockValueInsertAfterOperation:
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
    switch(static_cast<::OperationType>(type))
    {
      case ::OperationType::UndoGroupOperation:
      case ::OperationType::RedoGroupOperation:
      {
        auto op = reinterpret_cast<const UndoGroupOperation *>(this);
        return sizeof(*op) + op->length;
      }
      case ::OperationType::GroupOperation:
      case ::OperationType::AtomicGroupOperation:
      {
        auto op = reinterpret_cast<const GroupOperation *>(this);
        return sizeof(*op) + op->length;
      }
      case ::OperationType::SetAttributeOperation:
      {
        auto op = reinterpret_cast<const SetAttributeOperation *>(this);
        return sizeof(*op) + op->length;
      }
      case ::OperationType::NodeCreateOperation:
      {
        auto op = reinterpret_cast<const NodeCreateOperation *>(this);
        return sizeof(*op) + op->nodeTypeLength;
      }
      case ::OperationType::ValuePreviewOperation:
      case ::OperationType::ValueSetOperation:
      {
        auto op = reinterpret_cast<const ValueSetOperation *>(this);
        return sizeof(*op) + op->length;
      }
      case ::OperationType::BlockValueInsertAfterOperation:
      {
        auto op = reinterpret_cast<const BlockValueInsertAfterOperation *>(this);
        return sizeof(*op) + op->length;
      }
      default:
        return getStructSize();
    }
  }
}