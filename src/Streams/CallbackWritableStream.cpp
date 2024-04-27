#include "CallbackWritableStream.h"

template <class T>
CallbackWritableStream<T>::CallbackWritableStream(DataCallback dataCallback, EndCallback endCallback)
{
  this->dataCallback = dataCallback;
  this->endCallback = endCallback;
}

template <class T>
CallbackWritableStream<T>::CallbackWritableStream(DataCallback dataCallback)
{
  this->dataCallback = dataCallback;
  this->endCallback = [](){};
}

template <class T>
CallbackWritableStream<T>::CallbackWritableStream()
{
  this->dataCallback = [](const T & data){};
  this->endCallback = [](){};
}

template <class T>
bool CallbackWritableStream<T>::write(const T & data)
{
  dataCallback(data);
  return true;
}

template <class T>
void CallbackWritableStream<T>::close()
{
  if (closed)
  {
    return;
  }

  endCallback();
  closed = true;
}

#include "../LogOperation.h"
#include "../RefCounted.h"
#include <string>
template class CallbackWritableStream<RefCounted<const LogOperation>>;
template class CallbackWritableStream<std::string_view>;