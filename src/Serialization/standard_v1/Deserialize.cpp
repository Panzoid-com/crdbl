#include "Deserialize.h"
#include <cstring>

namespace Serialization_standard_v1
{
  void Deserialize(uint8_t * dst, const uint8_t * src, size_t length)
  {
    std::memcpy(dst, src, length);
  }

  template <class Dst, class Src>
  void Deserialize(Dst & dst, const Src & src)
  {
    dst = src;
  }

  template <>
  void Deserialize(::Tag & dst, const Tag & src)
  {
    Deserialize(dst.value, src.value);
  }

  template <>
  void Deserialize(::Timestamp & dst, const Timestamp & src)
  {
    Deserialize(dst.clock, src.clock);
    Deserialize(dst.site, src.site);
  }

  template <>
  void Deserialize(::NodeId & dst, const NodeId & src)
  {
    Deserialize(dst.ts, src.ts);
    Deserialize(dst.child, src.child);
  }

  template <>
  void Deserialize(::OperationType & dst, const OperationType & src)
  {
    dst = static_cast<::OperationType>(src);
  }

  template <>
  void Deserialize(::NoOpOperation & dst, const NoOpOperation & src)
  {
    /* No data */
  }

  template <>
  void Deserialize(::GroupOperation & dst, const GroupOperation & src)
  {
    Deserialize(dst.length, src.length);
    Deserialize(dst.data, src.data, src.length);
  }

  template <>
  void Deserialize(::UndoGroupOperation & dst, const UndoGroupOperation & src)
  {
    Deserialize(dst.prevTs, src.prevTs);
    Deserialize(dst.length, src.length);
    Deserialize(dst.data, src.data, src.length);
  }

  template <>
  void Deserialize(::AtomicGroupOperation & dst, const AtomicGroupOperation & src)
  {
    Deserialize(dst.length, src.length);
    Deserialize(dst.data, src.data, src.length);
  }

  template <>
  void Deserialize(::SetAttributeOperation & dst, const SetAttributeOperation & src)
  {
    Deserialize(dst.attributeId, src.attributeId);
    Deserialize(dst.length, src.length);
    Deserialize(dst.data, src.data, src.length);
  }

  template <>
  void Deserialize(::NodeCreateOperation & dst, const NodeCreateOperation & src)
  {
    Deserialize(dst.nodeTypeLength, src.nodeTypeLength);
    Deserialize(dst.data, src.data, src.nodeTypeLength);
  }

  template <>
  void Deserialize(::EdgeCreateOperation & dst, const EdgeCreateOperation & src)
  {
    Deserialize(dst.parentId, src.parentId);
    Deserialize(dst.childId, src.childId);
  }

  template <>
  void Deserialize(::UndoEdgeCreateOperation & dst, const UndoEdgeCreateOperation & src)
  {
    Deserialize(dst.parentId, src.parentId);
  }


  template <>
  void Deserialize(::EdgeDeleteOperation & dst, const EdgeDeleteOperation & src)
  {
    Deserialize(dst.parentId, src.parentId);
    Deserialize(dst.edgeId, src.edgeId);
  }

  template <>
  void Deserialize(::UndoEdgeDeleteOperation & dst, const UndoEdgeDeleteOperation & src)
  {
    Deserialize(dst.parentId, src.parentId);
    Deserialize(dst.edgeId, src.edgeId);
  }


  template <>
  void Deserialize(::ValueSetOperation & dst, const ValueSetOperation & src)
  {
    Deserialize(dst.nodeId, src.nodeId);
    Deserialize(dst.length, src.length);
    Deserialize(dst.data, src.data, src.length);
  }

  template <>
  void Deserialize(::UndoValueSetOperation & dst, const UndoValueSetOperation & src)
  {
    Deserialize(dst.nodeId, src.nodeId);
  }


  template <>
  void Deserialize(::BlockValueInsertAfterOperation & dst, const BlockValueInsertAfterOperation & src)
  {
    Deserialize(dst.nodeId, src.nodeId);
    Deserialize(dst.blockId, src.blockId);
    Deserialize(dst.offset, src.offset);
    Deserialize(dst.length, src.length);
    Deserialize(dst.data, src.data, src.length);
  }

  template <>
  void Deserialize(::UndoBlockValueInsertAfterOperation & dst, const UndoBlockValueInsertAfterOperation & src)
  {
    Deserialize(dst.nodeId, src.nodeId);
  }

  template <>
  void Deserialize(::BlockValueDeleteAfterOperation & dst, const BlockValueDeleteAfterOperation & src)
  {
    Deserialize(dst.nodeId, src.nodeId);
    Deserialize(dst.blockId, src.blockId);
    Deserialize(dst.offset, src.offset);
    Deserialize(dst.length, src.length);
  }

  template <>
  void Deserialize(::UndoBlockValueDeleteAfterOperation & dst, const UndoBlockValueDeleteAfterOperation & src)
  {
    Deserialize(dst.nodeId, src.nodeId);
    Deserialize(dst.blockId, src.blockId);
    Deserialize(dst.offset, src.offset);
    Deserialize(dst.length, src.length);
  }

  template <>
  void Deserialize(::Operation & dst, const Operation & src)
  {
    Deserialize(dst.type, src.type);

    switch(dst.type)
    {
      case ::OperationType::NoOpOperation:
      {
        Deserialize(*static_cast<::NoOpOperation *>(&dst),
          *static_cast<const NoOpOperation *>(&src));
        break;
      }
      case ::OperationType::UndoGroupOperation:
      case ::OperationType::RedoGroupOperation:
      {
        Deserialize(*static_cast<::UndoGroupOperation *>(&dst),
          *static_cast<const UndoGroupOperation *>(&src));
        break;
      }
      case ::OperationType::GroupOperation:
      case ::OperationType::AtomicGroupOperation:
      {
        Deserialize(*static_cast<::GroupOperation *>(&dst),
          *static_cast<const GroupOperation *>(&src));
        break;
      }
      case ::OperationType::SetAttributeOperation:
      {
        Deserialize(*static_cast<::SetAttributeOperation *>(&dst),
          *static_cast<const SetAttributeOperation *>(&src));
        break;
      }
      case ::OperationType::NodeCreateOperation:
      {
        Deserialize(*static_cast<::NodeCreateOperation *>(&dst),
          *static_cast<const NodeCreateOperation *>(&src));
        break;
      }
      case ::OperationType::EdgeCreateOperation:
      {
        Deserialize(*static_cast<::EdgeCreateOperation *>(&dst),
          *static_cast<const EdgeCreateOperation *>(&src));
        break;
      }
      case ::OperationType::UndoEdgeCreateOperation:
      {
        Deserialize(*static_cast<::UndoEdgeCreateOperation *>(&dst),
          *static_cast<const UndoEdgeCreateOperation *>(&src));
        break;
      }
      case ::OperationType::EdgeDeleteOperation:
      {
        Deserialize(*static_cast<::EdgeDeleteOperation *>(&dst),
          *static_cast<const EdgeDeleteOperation *>(&src));
        break;
      }
      case ::OperationType::UndoEdgeDeleteOperation:
      {
        Deserialize(*static_cast<::UndoEdgeDeleteOperation *>(&dst),
          *static_cast<const UndoEdgeDeleteOperation *>(&src));
        break;
      }
      case ::OperationType::ValuePreviewOperation:
      case ::OperationType::ValueSetOperation:
      {
        Deserialize(*static_cast<::ValueSetOperation *>(&dst),
          *static_cast<const ValueSetOperation *>(&src));
        break;
      }
      case ::OperationType::UndoValueSetOperation:
      {
        Deserialize(*static_cast<::UndoValueSetOperation *>(&dst),
          *static_cast<const UndoValueSetOperation *>(&src));
        break;
      }
      case ::OperationType::BlockValueInsertAfterOperation:
      {
        Deserialize(*static_cast<::BlockValueInsertAfterOperation *>(&dst),
          *static_cast<const BlockValueInsertAfterOperation *>(&src));
        break;
      }
      case ::OperationType::UndoBlockValueInsertAfterOperation:
      {
        Deserialize(*static_cast<::UndoBlockValueInsertAfterOperation *>(&dst),
          *static_cast<const UndoBlockValueInsertAfterOperation *>(&src));
        break;
      }
      case ::OperationType::BlockValueDeleteAfterOperation:
      {
        Deserialize(*static_cast<::BlockValueDeleteAfterOperation *>(&dst),
          *static_cast<const BlockValueDeleteAfterOperation *>(&src));
        break;
      }
      case ::OperationType::UndoBlockValueDeleteAfterOperation:
      {
        Deserialize(*static_cast<::UndoBlockValueDeleteAfterOperation *>(&dst),
          *static_cast<const UndoBlockValueDeleteAfterOperation *>(&src));
        break;
      }
      default:
        break;
    }
  }
};