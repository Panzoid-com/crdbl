#pragma once
#include "../LogOperationSerialization.h"
#include "../ILogOperationDeserializer.h"
#include "../../Streams/ReadableStreamBase.h"
#include "../../LogOperation.h"
#include "../../RefCounted.h"
// #include "LogOperation.h"
#include "Format.h"
#include <utility>
#include <string>

namespace Serialization_standard_v1
{
  template <Subformat F, DeserializeDirection D>
  class LogOperationDeserializer : public ILogOperationDeserializer
  {
  public:
    LogOperationDeserializer();
    ~LogOperationDeserializer() override;
    bool write(const std::string_view & data) override;
    void close() override;
    void pipeTo(IWritableStream<RefCounted<const ::LogOperation>> & writableStream) override;
  private:
    bool writeToDestination(const RefCounted<const ::LogOperation> & data);

    bool writeForward(const std::string_view & data);
    bool writeReverse(const std::string_view & data);

    static std::pair<RefCounted<const ::LogOperation>, size_t> DeserializeLogOperation(const std::string_view & data, size_t bufLength);

    bool closed = false;
    std::basic_string<char> streamBuffer;
    IWritableStream<RefCounted<const ::LogOperation>> * destination;
  };
};
