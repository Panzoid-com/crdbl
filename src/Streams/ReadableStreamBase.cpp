#include "ReadableStreamBase.h"

template <class T>
ReadableStreamBase<T>::~ReadableStreamBase()
{
  // if (destination != nullptr)
  // {
  //   delete destination;
  // }
}

template <class T>
bool ReadableStreamBase<T>::writeToDestination(const T & data)
{
  if (destination == nullptr)
  {
    return false;
  }

  return destination->write(data);
}

template <class T>
void ReadableStreamBase<T>::pipeTo(IWritableStream<T> & writableStream)
{
  destination = &writableStream;
}

#include "../LogOperation.h"
#include "../RefCounted.h"
#include <string>
template class ReadableStreamBase<RefCounted<const LogOperation>>;
template class ReadableStreamBase<std::string_view>;