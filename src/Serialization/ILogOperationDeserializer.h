#pragma once
#include "../Streams/IReadableStream.h"
#include "../Streams/IWritableStream.h"
#include "../LogOperation.h"
#include "../RefCounted.h"
#include <string>

class ILogOperationDeserializer : public IWritableStream<std::string_view>, public IReadableStream<RefCounted<const LogOperation>>
{
public:
  //NOTE: possible future interface feature
  // virtual int errorsDetected() = 0;
  // virtual int errorsCorrected() = 0;
  virtual ~ILogOperationDeserializer() = default;
};