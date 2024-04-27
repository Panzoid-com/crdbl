#pragma once
#include "IReadableStream.h"

class OperationBuilder;
class OperationLog;

template <class T>
class ReadableStreamBase : public IReadableStream<T>
{
public:
  ~ReadableStreamBase() override;
  void pipeTo(IWritableStream<T> & writableStream) override;
protected:
  bool writeToDestination(const T & data);
  IWritableStream<T> * destination = nullptr;

  friend class OperationBuilder;
  friend class OperationLog;
};