#pragma once
#include "../Streams/IReadableStream.h"
#include "../Streams/IWritableStream.h"
#include "../LogOperation.h"
#include "../RefCounted.h"
#include <string>

class ILogOperationSerializer : public IWritableStream<RefCounted<const LogOperation>>, public IReadableStream<std::string_view>
{
public:
  virtual void setOutputSize(size_t size) = 0;
  virtual ~ILogOperationSerializer() = default;
};