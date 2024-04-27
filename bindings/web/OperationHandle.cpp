#include "OperationHandle.h"

OperationHandle::OperationHandle(LogOperation * operation)
{
  op = operation;
}

LogOperation * OperationHandle::getOperationPointer()
{
  return op;
}

void OperationHandle::invalidate()
{
  op = nullptr;
}

val OperationHandle::asArray()
{
  val in_arr = val(typed_memory_view(size(), static_cast<const uint8_t *>(op)));
  // val out_arr = val::global("Uint8Array").new_(in_arr);

  return in_arr;
}

bool OperationHandle::isUndo()
{
  return op->op.isUndo();
}

bool OperationHandle::isRedo()
{
  return op->op.isRedo();
}

bool OperationHandle::isPreview()
{
  return op->op.type == OperationType::ValuePreviewOperation;
}

OperationType OperationHandle::getType()
{
  return op->op.type;
}

uint32_t OperationHandle::getSiteId()
{
  return op->ts.site;
}

uint32_t OperationHandle::getClock()
{
  return op->ts.clock;
}

size_t OperationHandle::size()
{
  return op->getSize();
}