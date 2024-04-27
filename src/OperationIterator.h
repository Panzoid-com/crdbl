#pragma once
#include "Operation.h"

class OperationIterator
{
public:
  OperationIterator(const Operation * start, size_t length);
  OperationIterator & operator++();
  const Operation * operator*() const;

private:
  const char * data;
  size_t dataLength;
};