#include "LogOperation.h"

size_t LogOperation::getSize() const
{
  return getSizeWithoutOp() + op.getSize();
}