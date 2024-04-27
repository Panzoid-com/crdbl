#include "OperationIterator.h"

OperationIterator::OperationIterator(const Operation * start, size_t length)
{
  if (start == nullptr || length == 0)
  {
    data = nullptr;
    dataLength = 0;
    return;
  }

  dataLength = length;

  //validate first operation size
  size_t opLength = start->getSize();
  if (opLength > dataLength || opLength == 0)
  {
    data = nullptr;
    return;
  }

  data = reinterpret_cast<const char *>(start);
}

OperationIterator & OperationIterator::operator++()
{
  size_t length = reinterpret_cast<const Operation *>(data)->getSize();

  if (length >= dataLength || length == 0)
  {
    data = nullptr;
    return *this;
  }

  data += length;
  dataLength -= length;

  //validate current operation size
  length = reinterpret_cast<const Operation *>(data)->getSize();
  if (length > dataLength || length == 0)
  {
    data = nullptr;
  }

  return *this;
}

const Operation * OperationIterator::operator*() const
{
  return reinterpret_cast<const Operation *>(data);
}