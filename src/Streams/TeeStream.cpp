#include "TeeStream.h"

template <class T>
TeeStream<T>::TeeStream(IWritableStream<T> & destA, IWritableStream<T> & destB)
  : CallbackWritableStream<T>(
    [&](const T & data)
    {
      _destA->write(data);
      _destB->write(data);
    },
    [](){}),
    _destA(&destA), _destB(&destB)
{}

#include "../LogOperation.h"
#include "../RefCounted.h"
#include <string>
template class TeeStream<RefCounted<const LogOperation>>;
template class TeeStream<std::string_view>;