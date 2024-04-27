#pragma once
#include "IWritableStream.h"

template <class T>
class CallbackWritableStream : public IWritableStream<T>
{
public:
  using DataCallback = std::function<void(const T & data)>;
  using EndCallback = std::function<void()>;

  CallbackWritableStream(DataCallback dataCallback, EndCallback endCallback);
  CallbackWritableStream(DataCallback dataCallback);
  CallbackWritableStream();

  bool write(const T & data) override;
  void close() override;
protected:
  DataCallback dataCallback;
  EndCallback endCallback;
  bool closed = false;
};