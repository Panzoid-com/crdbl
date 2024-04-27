#pragma once
#include <cstdint>
#include "Timestamp.h"
#include "Tag.h"
#include "NodeId.h"
#include "EdgeId.h"
#include "OperationType.h"

#pragma pack(1) //ideally in a future iteration we shouldn't need to pack these

struct Operation
{
  OperationType type;

  static size_t getStructSize(OperationType type);
  static size_t getSize(OperationType type, size_t dataSize);

  size_t getStructSize() const;
  size_t getDataSize() const;
  size_t getSize() const;
  void transformTimestamps(const Timestamp & offset);
  bool isUndo() const;
  bool isRedo() const;
  bool isGroup() const;
  uint32_t getMaxTsOffset() const;

  bool operator==(const Operation & rhs) const;
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

  UndoEdgeCreateOperation(const EdgeCreateOperation * op)
  {
    type = OperationType::UndoEdgeCreateOperation;
    parentId = op->parentId;
  }

  UndoEdgeCreateOperation(const UndoEdgeCreateOperation * op)
  {
    if (op->type == OperationType::UndoEdgeCreateOperation)
    {
      type = OperationType::RedoEdgeCreateOperation;
    }
    else
    {
      type = OperationType::UndoEdgeCreateOperation;
    }
    parentId = op->parentId;
  }
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

  UndoEdgeDeleteOperation(const EdgeDeleteOperation * op)
  {
    type = OperationType::UndoEdgeDeleteOperation;
    parentId = op->parentId;
    edgeId = op->edgeId;
  }

  UndoEdgeDeleteOperation(const UndoEdgeDeleteOperation * op)
  {
    if (op->type == OperationType::UndoEdgeDeleteOperation)
    {
      type = OperationType::RedoEdgeDeleteOperation;
    }
    else
    {
      type = OperationType::UndoEdgeDeleteOperation;
    }
    parentId = op->parentId;
    edgeId = op->edgeId;
  }
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

  UndoValueSetOperation(const ValueSetOperation * op)
  {
    type = OperationType::UndoValueSetOperation;
    nodeId = op->nodeId;
  }

  UndoValueSetOperation(const UndoValueSetOperation * op)
  {
    if (op->type == OperationType::UndoValueSetOperation)
    {
      type = OperationType::RedoValueSetOperation;
    }
    else
    {
      type = OperationType::UndoValueSetOperation;
    }
    nodeId = op->nodeId;
  }
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

  UndoBlockValueInsertAfterOperation(const BlockValueInsertAfterOperation * op)
  {
    type = OperationType::UndoBlockValueInsertAfterOperation;
    nodeId = op->nodeId;
  }

  UndoBlockValueInsertAfterOperation(const UndoBlockValueInsertAfterOperation * op)
  {
    if (op->type == OperationType::UndoBlockValueInsertAfterOperation)
    {
      type = OperationType::RedoBlockValueInsertAfterOperation;
    }
    else
    {
      type = OperationType::UndoBlockValueInsertAfterOperation;
    }
    nodeId = op->nodeId;
  }
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

  UndoBlockValueDeleteAfterOperation(const BlockValueDeleteAfterOperation * op)
  {
    type = OperationType::UndoBlockValueDeleteAfterOperation;
    blockId = op->blockId;
    nodeId = op->nodeId;
    offset = op->offset;
    length = op->length;
  }

  UndoBlockValueDeleteAfterOperation(const UndoBlockValueDeleteAfterOperation * op)
  {
    if (op->type == OperationType::UndoBlockValueDeleteAfterOperation)
    {
      type = OperationType::RedoBlockValueDeleteAfterOperation;
    }
    else
    {
      type = OperationType::UndoBlockValueDeleteAfterOperation;
    }
    blockId = op->blockId;
    nodeId = op->nodeId;
    offset = op->offset;
    length = op->length;
  }
};

#pragma options align=reset