#pragma once
#include <emscripten/bind.h>
#include <Streams/CallbackWritableStream.h>
#include <Streams/FilterOperationStream.h>
#include <Serialization/LogOperationSerialization.h>
#include <OperationBuilder.h>
#include <OperationLog.h>
#include "StringIds.h"
#include <string>
#include <ctime>

OperationFilter * FilterOperationStream_getFilter(FilterOperationStream & ref)
{
  return &ref.getFilter();
}

template <typename R, class C, auto F, typename... Args>
R WrapReturnValue(C & ref, Args... args)
{
  return static_cast<R>(std::invoke(F, ref, args...));
}

std::string LogOperationSerialization_DefaultFormat()
{
  return LogOperationSerialization::DefaultFormat();
}

std::string LogOperationSerialization_DefaultTypeFormat()
{
  return LogOperationSerialization::DefaultTypeFormat();
}

CallbackWritableStream<std::string_view> * DataCallbackStream_Create(val dataCallback, val endCallback)
{
  auto callbackStream = new CallbackWritableStream<std::string_view>(
    [dataCallback](const std::string_view & data)
    {
      val out_arr = val(typed_memory_view(data.size(), reinterpret_cast<const char *>(data.data())));
      // val out_arr = val::global("Uint8Array").new_(in_arr);
      dataCallback(out_arr);
    },
    [endCallback]()
    {
      endCallback();
    }
  );

  return callbackStream;
}

CallbackWritableStream<RefCounted<const LogOperation>> * LogOperationCallbackStream_Create(val dataCallback, val endCallback)
{
  auto callbackStream = new CallbackWritableStream<RefCounted<const LogOperation>>(
    [dataCallback](const RefCounted<const LogOperation> & data)
    {
      dataCallback(data);
    },
    [endCallback]()
    {
      endCallback();
    }
  );

  return callbackStream;
}

std::time_t std_time_0()
{
  return std::time(0);
}

template <typename T, typename R>
IReadableStream<R> * ITransformStream_asReadable(T & ref)
{
  return static_cast<IReadableStream<R> *>(&ref);
}

template <typename T, typename R>
IWritableStream<R> * ITransformStream_asWritable(T & ref)
{
  return static_cast<IWritableStream<R> *>(&ref);
}

bool WritableDataStream_write(IWritableStream<std::string_view> & ref, val arrayBufferView)
{
  const auto size = arrayBufferView["byteLength"].as<unsigned>();
  const uint8_t * data = new uint8_t[size];

  val uint8Array = val::global("Uint8Array").new_(arrayBufferView["buffer"],
    arrayBufferView["byteOffset"], arrayBufferView["byteLength"]);
  val memoryView{typed_memory_view(size, data)};
  memoryView.call<void>("set", uint8Array);

  bool result = ref.write(std::string_view(reinterpret_cast<const char *>(data), size));

  delete[] data;

  return result;
}

VectorTimestamp * OperationLog_getVectorClock(const OperationLog & ref)
{
  //NOTE: not great, not clear what the better embind solution is
  return const_cast<VectorTimestamp *>(&ref.getVectorClock());
}