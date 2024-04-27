#pragma once
#include "../Operation.h"

template <class T>
class IWritableStream
{
public:
  virtual bool write(const T & data) = 0;
  virtual void close() = 0;
  virtual ~IWritableStream() = default;
};