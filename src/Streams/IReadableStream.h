#pragma once
#include "IWritableStream.h"

template <class T>
class IReadableStream
{
public:
  virtual void pipeTo(IWritableStream<T> & writableStream) = 0;
  virtual ~IReadableStream() = default;
};