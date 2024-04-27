#pragma once
#include <cstdint>
#include "../../OperationType.h"
#include "../../Operation.h"
#include "../../LogOperation.h"

#pragma pack(1)

namespace Serialization_standard_v1
{
  struct Tag
  {
    std::array<uint32_t, 4> value;
  };

  struct Timestamp
  {
    uint32_t clock;
    uint32_t site;
  };

  struct NodeId
  {
    Timestamp ts;
    uint32_t child;
  };

  using EdgeId = NodeId;

  using OperationType = uint8_t;

  struct Operation
  {
    OperationType type;

    static size_t getStructSize(OperationType type);
    static size_t getSize(OperationType type, size_t dataSize);

    size_t getStructSize() const;
    size_t getDataSize() const;
    size_t getSize() const;
  };

  struct NoOpOperation : public Operation
  {
    /* No data */
  };

  struct GroupOperation : public Operation
  {
    uint32_t length;
    uint8_t data[];
  };

  struct UndoGroupOperation : public Operation
  {
    Timestamp prevTs;
    uint32_t length;
    uint8_t data[];
  };

  struct AtomicGroupOperation : public Operation
  {
    uint32_t length;
    uint8_t data[];
  };

  struct SetAttributeOperation : public Operation
  {
    uint8_t attributeId;
    uint16_t length;
    uint8_t data[];
  };

  struct NodeCreateOperation : public Operation
  {
    uint8_t nodeTypeLength;
    uint8_t data[];
  };

  struct EdgeCreateOperation : public Operation
  {
    NodeId parentId;
    NodeId childId;
  };

  struct UndoEdgeCreateOperation : public Operation
  {
    NodeId parentId;
  };


  struct EdgeDeleteOperation : public Operation
  {
    NodeId parentId;
    EdgeId edgeId;
  };

  struct UndoEdgeDeleteOperation : public Operation
  {
    NodeId parentId;
    EdgeId edgeId;
  };


  struct ValueSetOperation : public Operation
  {
    NodeId nodeId;
    uint32_t length;
    uint8_t data[];
  };

  struct UndoValueSetOperation : public Operation
  {
    NodeId nodeId;
  };


  struct BlockValueInsertAfterOperation : public Operation
  {
    NodeId nodeId;
    Timestamp blockId;
    uint32_t offset;
    uint32_t length;
    uint8_t data[];
  };

  struct UndoBlockValueInsertAfterOperation : public Operation
  {
    NodeId nodeId;
  };

  struct BlockValueDeleteAfterOperation : public Operation
  {
    NodeId nodeId;
    Timestamp blockId;
    uint32_t offset;
    uint32_t length;
  };

  struct UndoBlockValueDeleteAfterOperation : public Operation
  {
    NodeId nodeId;
    Timestamp blockId;
    uint32_t offset;
    uint32_t length;
  };

  struct LogOperationFull
  {
    Tag tag;
    Timestamp ts;
    Operation op;

    static constexpr size_t getHeaderSize() { return sizeof(Tag) + sizeof(Timestamp); }
    static constexpr size_t getFooterSize() { return sizeof(uint32_t); }
    static constexpr size_t getSizeWithoutOp() { return getHeaderSize() + getFooterSize(); }
    static constexpr size_t getStructSize() { return getSizeWithoutOp() + sizeof(Operation); }

    static size_t getSize(OperationType type, size_t dataSize) { return getSizeWithoutOp() + Operation::getSize(type, dataSize); }

    size_t getSize() const { return getSizeWithoutOp() + op.getSize(); }
  };

  struct LogOperationUntagged
  {
    Timestamp ts;
    Operation op;

    static constexpr size_t getHeaderSize() { return sizeof(Timestamp); }
    static constexpr size_t getFooterSize() { return sizeof(uint32_t); }
    static constexpr size_t getSizeWithoutOp() { return getHeaderSize() + getFooterSize(); }
    static constexpr size_t getStructSize() { return getSizeWithoutOp() + sizeof(Operation); }

    static size_t getSize(OperationType type, size_t dataSize) { return getSizeWithoutOp() + Operation::getSize(type, dataSize); }

    size_t getSize() const { return getSizeWithoutOp() + op.getSize(); }
  };

  struct LogOperationForward
  {
    Tag tag;
    Timestamp ts;
    Operation op;

    static constexpr size_t getHeaderSize() { return sizeof(Tag) + sizeof(Timestamp); }
    static constexpr size_t getFooterSize() { return 0; }
    static constexpr size_t getSizeWithoutOp() { return getHeaderSize() + getFooterSize(); }
    static constexpr size_t getStructSize() { return getSizeWithoutOp() + sizeof(Operation); }

    static size_t getSize(OperationType type, size_t dataSize) { return getSizeWithoutOp() + Operation::getSize(type, dataSize); }

    size_t getSize() const { return getSizeWithoutOp() + op.getSize(); }
  };

  struct LogOperationType
  {
    Operation op;

    static constexpr size_t getHeaderSize() { return 0; }
    static constexpr size_t getFooterSize() { return 0; }
    static constexpr size_t getSizeWithoutOp() { return getHeaderSize() + getFooterSize(); }
    static constexpr size_t getStructSize() { return getSizeWithoutOp() + sizeof(Operation); }

    static size_t getSize(OperationType type, size_t dataSize) { return getSizeWithoutOp() + Operation::getSize(type, dataSize); }

    size_t getSize() const { return getSizeWithoutOp() + op.getSize(); }
  };
};

#pragma options align=reset