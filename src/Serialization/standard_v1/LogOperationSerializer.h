#pragma once
#include "../LogOperationSerialization.h"
#include "../ILogOperationSerializer.h"
#include "../../Streams/ReadableStreamBase.h"
#include "../../LogOperation.h"
// #include "LogOperation.h"
#include "Format.h"
#include "../../RefCounted.h"
#include <string>

namespace Serialization_standard_v1
{
  template <Subformat F>
  class LogOperationSerializer : public ILogOperationSerializer
  {
  public:
    LogOperationSerializer(size_t headerPadding) noexcept;
    ~LogOperationSerializer() override;
    bool write(const RefCounted<const ::LogOperation> & data) override;
    void close() override;
    void pipeTo(IWritableStream<std::string_view> & writableStream) override;
    void setOutputSize(size_t size) override;

  private:
    bool writeToDestination(const std::string_view & data);

    static size_t SerializeLogOperation(const RefCounted<const ::LogOperation> & data, std::basic_string<char> & buffer);

    bool closed = false;
    size_t headerPadding;
    size_t outputSize = 0;
    std::basic_string<char> streamBuffer;
    IWritableStream<std::string_view> * destination;
  };
};
