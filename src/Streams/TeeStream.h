#pragma once
#include "CallbackWritableStream.h"
#include "IWritableStream.h"

template <class T>
class TeeStream final : public CallbackWritableStream<T>
{
public:
  TeeStream(IWritableStream<T> & destA, IWritableStream<T> & destB);

private:
  IWritableStream<T> * _destA;
  IWritableStream<T> * _destB;
};