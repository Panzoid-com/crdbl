#pragma once
#include <cstdint>

enum class OperationType : uint8_t
{
  NoOpOperation,

  GroupOperation,
  AtomicGroupOperation,
  UndoGroupOperation,
  RedoGroupOperation,

  SetAttributeOperation,

  NodeCreateOperation,

  EdgeCreateOperation,
  UndoEdgeCreateOperation,
  RedoEdgeCreateOperation,

  EdgeDeleteOperation,
  UndoEdgeDeleteOperation,
  RedoEdgeDeleteOperation,

  ValuePreviewOperation,
  ValueSetOperation,
  UndoValueSetOperation,
  RedoValueSetOperation,

  BlockValueInsertAfterOperation,
  UndoBlockValueInsertAfterOperation,
  RedoBlockValueInsertAfterOperation,

  BlockValueDeleteAfterOperation,
  UndoBlockValueDeleteAfterOperation,
  RedoBlockValueDeleteAfterOperation
};