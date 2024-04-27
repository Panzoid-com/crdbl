#include "LogOperationSerializer.h"
#include "LogOperation.h"

#include "Serialize.cpp"

//arbitrary restriction to limit the configured size of buffered data to
//  something reasonable
static constexpr size_t MaxOutputSize = 1 << 26;

namespace Serialization_standard_v1
{
  template<Subformat F>
  LogOperationSerializer<F>::LogOperationSerializer(size_t _headerPadding) noexcept
    : headerPadding(_headerPadding)
  {
    // streamBuffer.reserve(bufferSize);
  }

  template<Subformat F>
  LogOperationSerializer<F>::~LogOperationSerializer()
  {
    close();
  }

  template <Subformat F>
  bool LogOperationSerializer<F>::write(const RefCounted<const ::LogOperation> & data)
  {
    if (data == nullptr || closed)
    {
      return true;
    }

    //TODO: it might be nice to make this templated/static
    if (streamBuffer.size() == 0 && headerPadding > 0)
    {
      streamBuffer.append(headerPadding, '\0');
    }

    SerializeLogOperation(data, streamBuffer);

    if (streamBuffer.size() >= outputSize)
    {
      writeToDestination(std::string_view(streamBuffer.data(), streamBuffer.size()));
      streamBuffer.clear();
    }

    return true;
  }

  template <Subformat F>
  void LogOperationSerializer<F>::close()
  {
    if (closed)
    {
      return;
    }

    if (streamBuffer.size() > 0)
    {
      writeToDestination(std::string_view(streamBuffer.data(), streamBuffer.size()));
      streamBuffer.clear();
    }
  }

  template <Subformat F>
  bool LogOperationSerializer<F>::writeToDestination(const std::string_view & data)
  {
    if (destination == nullptr)
    {
      return false;
    }

    return destination->write(data);
  }

  template <Subformat F>
  void LogOperationSerializer<F>::pipeTo(IWritableStream<std::string_view> & writableStream)
  {
    destination = &writableStream;
  }

  template <Subformat F>
  void LogOperationSerializer<F>::setOutputSize(size_t size)
  {
    if (size > MaxOutputSize)
    {
      size = MaxOutputSize;
    }
    outputSize = size;
  }

  template <>
  size_t LogOperationSerializer<Subformat::Full>::SerializeLogOperation(const RefCounted<const ::LogOperation> & data, std::basic_string<char> & buffer)
  {
    OperationType opType;
    Serialize(opType, data->op.type);

    size_t opDataSize = LogOperationFull::getSize(opType, data->op.getDataSize());

    buffer.append(opDataSize, '\0');
    auto serializedOp = reinterpret_cast<LogOperationFull *>(buffer.data() + buffer.size() - opDataSize);

    Serialize(serializedOp->ts, data->ts);
    Serialize(serializedOp->tag, data->tag);
    Serialize(serializedOp->op, data->op);
    SetLogOperationFooter(*serializedOp, opDataSize);

    return opDataSize;
  }

  template <>
  size_t LogOperationSerializer<Subformat::Untagged>::SerializeLogOperation(const RefCounted<const ::LogOperation> & data, std::basic_string<char> & buffer)
  {
    OperationType opType;
    Serialize(opType, data->op.type);

    size_t opDataSize = LogOperationUntagged::getSize(opType, data->op.getDataSize());

    buffer.append(opDataSize, '\0');
    auto serializedOp = reinterpret_cast<LogOperationUntagged *>(buffer.data() + buffer.size() - opDataSize);

    Serialize(serializedOp->ts, data->ts);
    Serialize(serializedOp->op, data->op);
    SetLogOperationFooter(*serializedOp, opDataSize);

    return opDataSize;
  }

  template <>
  size_t LogOperationSerializer<Subformat::Forward>::SerializeLogOperation(const RefCounted<const ::LogOperation> & data, std::basic_string<char> & buffer)
  {
    OperationType opType;
    Serialize(opType, data->op.type);

    size_t opDataSize = LogOperationForward::getSize(opType, data->op.getDataSize());

    buffer.append(opDataSize, '\0');
    auto serializedOp = reinterpret_cast<LogOperationForward *>(buffer.data() + buffer.size() - opDataSize);

    Serialize(serializedOp->ts, data->ts);
    Serialize(serializedOp->tag, data->tag);
    Serialize(serializedOp->op, data->op);

    return opDataSize;
  }

  template <>
  size_t LogOperationSerializer<Subformat::Type>::SerializeLogOperation(const RefCounted<const ::LogOperation> & data, std::basic_string<char> & buffer)
  {
    OperationType opType;
    Serialize(opType, data->op.type);

    size_t opDataSize = LogOperationType::getSize(opType, data->op.getDataSize());

    buffer.append(opDataSize, '\0');
    auto serializedOp = reinterpret_cast<LogOperationType *>(buffer.data() + buffer.size() - opDataSize);

    Serialize(serializedOp->op, data->op);

    return opDataSize;
  }

  template class LogOperationSerializer<Subformat::Full>;
  template class LogOperationSerializer<Subformat::Untagged>;
  template class LogOperationSerializer<Subformat::Forward>;
  template class LogOperationSerializer<Subformat::Type>;
};
