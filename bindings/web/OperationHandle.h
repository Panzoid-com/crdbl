#pragma once
#include <emscripten/bind.h>
#include <Operation.h>

class OperationHandle
{
  OperationHandle(LogOperation * operation);

  LogOperation * getOperationPointer();
  void invalidate();

  val asArray();

  bool isUndo();
  bool isRedo();
  bool isPreview();
  // bool isGroup();
  OperationType getType();
  uint32_t getSiteId();
  uint32_t getClock();
  size_t size();

private:
  LogOperation * op = nullptr;
};