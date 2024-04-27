#pragma once
#include "Operation.h"
#include "Timestamp.h"
#include "Tag.h"

struct LogOperation
{
  Tag tag;
  Timestamp ts;
  Operation op;

  static constexpr size_t getSizeWithoutOp()
  {
    return offsetof(LogOperation, op);
  }

  static size_t getSize(OperationType type, size_t dataSize)
  {
    return getSizeWithoutOp() + Operation::getSize(type, dataSize);
  }

  size_t getSize() const;

  bool operator==(const LogOperation & rhs) const
  {
    return tag == rhs.tag && ts == rhs.ts && op == rhs.op;
  }
};