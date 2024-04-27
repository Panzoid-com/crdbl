#include "Serialize.h"
#include <cstring>

namespace Serialization_standard_v1
{
  void Serialize(uint8_t * dst, const uint8_t * src, size_t length)
  {
    std::memcpy(dst, src, length);
  }

  template <class D, class S>
  void Serialize(D & dst, const S & src)
  {
    dst = src;
  }

  template <>
  void Serialize(Tag & dst, const ::Tag & src)
  {
    Serialize(dst.value, src.value);
  }

  template <>
  void Serialize(Timestamp & dst, const ::Timestamp & src)
  {
    Serialize(dst.clock, src.clock);
    Serialize(dst.site, src.site);
  }

  template <>
  void Serialize(NodeId & dst, const ::NodeId & src)
  {
    Serialize(dst.ts, src.ts);
    Serialize(dst.child, src.child);
  }

  template <>
  void Serialize(OperationType & dst, const ::OperationType & src)
  {
    dst = static_cast<uint8_t>(src);
  }

  template <>
  void Serialize(NoOpOperation & dst, const ::NoOpOperation & src)
  {
    /* No data */
  }

  template <>
  void Serialize(GroupOperation & dst, const ::GroupOperation & src)
  {
    Serialize(dst.length, src.length);
    Serialize(dst.data, src.data, src.length);
  }

  template <>
  void Serialize(UndoGroupOperation & dst, const ::UndoGroupOperation & src)
  {
    Serialize(dst.prevTs, src.prevTs);
    Serialize(dst.length, src.length);
    Serialize(dst.data, src.data, src.length);
  }

  template <>
  void Serialize(AtomicGroupOperation & dst, const ::AtomicGroupOperation & src)
  {
    Serialize(dst.length, src.length);
    Serialize(dst.data, src.data, src.length);
  }

  template <>
  void Serialize(SetAttributeOperation & dst, const ::SetAttributeOperation & src)
  {
    Serialize(dst.attributeId, src.attributeId);
    Serialize(dst.length, src.length);
    Serialize(dst.data, src.data, src.length);
  }

  template <>
  void Serialize(NodeCreateOperation & dst, const ::NodeCreateOperation & src)
  {
    Serialize(dst.nodeTypeLength, src.nodeTypeLength);
    Serialize(dst.data, src.data, src.nodeTypeLength);
  }

  template <>
  void Serialize(EdgeCreateOperation & dst, const ::EdgeCreateOperation & src)
  {
    Serialize(dst.parentId, src.parentId);
    Serialize(dst.childId, src.childId);
  }

  template <>
  void Serialize(UndoEdgeCreateOperation & dst, const ::UndoEdgeCreateOperation & src)
  {
    Serialize(dst.parentId, src.parentId);
  }


  template <>
  void Serialize(EdgeDeleteOperation & dst, const ::EdgeDeleteOperation & src)
  {
    Serialize(dst.parentId, src.parentId);
    Serialize(dst.edgeId, src.edgeId);
  }

  template <>
  void Serialize(UndoEdgeDeleteOperation & dst, const ::UndoEdgeDeleteOperation & src)
  {
    Serialize(dst.parentId, src.parentId);
    Serialize(dst.edgeId, src.edgeId);
  }


  template <>
  void Serialize(ValueSetOperation & dst, const ::ValueSetOperation & src)
  {
    Serialize(dst.nodeId, src.nodeId);
    Serialize(dst.length, src.length);
    Serialize(dst.data, src.data, src.length);
  }

  template <>
  void Serialize(UndoValueSetOperation & dst, const ::UndoValueSetOperation & src)
  {
    Serialize(dst.nodeId, src.nodeId);
  }


  template <>
  void Serialize(BlockValueInsertAfterOperation & dst, const ::BlockValueInsertAfterOperation & src)
  {
    Serialize(dst.nodeId, src.nodeId);
    Serialize(dst.blockId, src.blockId);
    Serialize(dst.offset, src.offset);
    Serialize(dst.length, src.length);
    Serialize(dst.data, src.data, src.length);
  }

  template <>
  void Serialize(UndoBlockValueInsertAfterOperation & dst, const ::UndoBlockValueInsertAfterOperation & src)
  {
    Serialize(dst.nodeId, src.nodeId);
  }

  template <>
  void Serialize(BlockValueDeleteAfterOperation & dst, const ::BlockValueDeleteAfterOperation & src)
  {
    Serialize(dst.nodeId, src.nodeId);
    Serialize(dst.blockId, src.blockId);
    Serialize(dst.offset, src.offset);
    Serialize(dst.length, src.length);
  }

  template <>
  void Serialize(UndoBlockValueDeleteAfterOperation & dst, const ::UndoBlockValueDeleteAfterOperation & src)
  {
    Serialize(dst.nodeId, src.nodeId);
    Serialize(dst.blockId, src.blockId);
    Serialize(dst.offset, src.offset);
    Serialize(dst.length, src.length);
  }

  template <>
  void Serialize(Operation & dst, const ::Operation & src)
  {
    Serialize(dst.type, src.type);

    switch(src.type)
    {
      case ::OperationType::NoOpOperation:
      {
        Serialize(*static_cast<NoOpOperation *>(&dst),
          *static_cast<const ::NoOpOperation *>(&src));
        break;
      }
      case ::OperationType::UndoGroupOperation:
      case ::OperationType::RedoGroupOperation:
      {
        Serialize(*static_cast<UndoGroupOperation *>(&dst),
          *static_cast<const ::UndoGroupOperation *>(&src));
        break;
      }
      case ::OperationType::GroupOperation:
      case ::OperationType::AtomicGroupOperation:
      {
        Serialize(*static_cast<GroupOperation *>(&dst),
          *static_cast<const ::GroupOperation *>(&src));
        break;
      }
      case ::OperationType::SetAttributeOperation:
      {
        Serialize(*static_cast<SetAttributeOperation *>(&dst),
          *static_cast<const ::SetAttributeOperation *>(&src));
        break;
      }
      case ::OperationType::NodeCreateOperation:
      {
        Serialize(*static_cast<NodeCreateOperation *>(&dst),
          *static_cast<const ::NodeCreateOperation *>(&src));
        break;
      }
      case ::OperationType::EdgeCreateOperation:
      {
        Serialize(*static_cast<EdgeCreateOperation *>(&dst),
          *static_cast<const ::EdgeCreateOperation *>(&src));
        break;
      }
      case ::OperationType::UndoEdgeCreateOperation:
      {
        Serialize(*static_cast<UndoEdgeCreateOperation *>(&dst),
          *static_cast<const ::UndoEdgeCreateOperation *>(&src));
        break;
      }
      case ::OperationType::EdgeDeleteOperation:
      {
        Serialize(*static_cast<EdgeDeleteOperation *>(&dst),
          *static_cast<const ::EdgeDeleteOperation *>(&src));
        break;
      }
      case ::OperationType::UndoEdgeDeleteOperation:
      {
        Serialize(*static_cast<UndoEdgeDeleteOperation *>(&dst),
          *static_cast<const ::UndoEdgeDeleteOperation *>(&src));
        break;
      }
      case ::OperationType::ValuePreviewOperation:
      case ::OperationType::ValueSetOperation:
      {
        Serialize(*static_cast<ValueSetOperation *>(&dst),
          *static_cast<const ::ValueSetOperation *>(&src));
        break;
      }
      case ::OperationType::UndoValueSetOperation:
      {
        Serialize(*static_cast<UndoValueSetOperation *>(&dst),
          *static_cast<const ::UndoValueSetOperation *>(&src));
        break;
      }
      case ::OperationType::BlockValueInsertAfterOperation:
      {
        Serialize(*static_cast<BlockValueInsertAfterOperation *>(&dst),
          *static_cast<const ::BlockValueInsertAfterOperation *>(&src));
        break;
      }
      case ::OperationType::UndoBlockValueInsertAfterOperation:
      {
        Serialize(*static_cast<UndoBlockValueInsertAfterOperation *>(&dst),
          *static_cast<const ::UndoBlockValueInsertAfterOperation *>(&src));
        break;
      }
      case ::OperationType::BlockValueDeleteAfterOperation:
      {
        Serialize(*static_cast<BlockValueDeleteAfterOperation *>(&dst),
          *static_cast<const ::BlockValueDeleteAfterOperation *>(&src));
        break;
      }
      case ::OperationType::UndoBlockValueDeleteAfterOperation:
      {
        Serialize(*static_cast<UndoBlockValueDeleteAfterOperation *>(&dst),
          *static_cast<const ::UndoBlockValueDeleteAfterOperation *>(&src));
        break;
      }
      default:
        break;
    }
  }

  template <>
  void SetLogOperationFooter(LogOperationFull & logOperation, size_t opSize)
  {
    uint32_t * footer = reinterpret_cast<uint32_t *>(
      reinterpret_cast<uint8_t *>(&logOperation) + opSize - sizeof(uint32_t));
    *footer = opSize;
  }

  template <>
  void SetLogOperationFooter(LogOperationUntagged & logOperation, size_t opSize)
  {
    uint32_t * footer = reinterpret_cast<uint32_t *>(
      reinterpret_cast<uint8_t *>(&logOperation) + opSize - sizeof(uint32_t));
    *footer = opSize;
  }
};