#include "LogOperationDeserializer.h"
#include "LogOperation.h"

#include "Deserialize.cpp"

namespace Serialization_standard_v1
{
  template <Subformat F, DeserializeDirection D>
  LogOperationDeserializer<F, D>::LogOperationDeserializer()
  {

  }

  template <Subformat F, DeserializeDirection D>
  LogOperationDeserializer<F, D>::~LogOperationDeserializer()
  {
    close();

    if (destination != nullptr)
    {
      // delete destination;
    }
  }

  template <Subformat F, DeserializeDirection D>
  bool LogOperationDeserializer<F, D>::write(const std::string_view & data)
  {
    if constexpr (D == DeserializeDirection::Forward)
    {
      return writeForward(data);
    }
    else
    {
      return writeReverse(data);
    }
  }

  template <Subformat F, DeserializeDirection D>
  bool LogOperationDeserializer<F, D>::writeForward(const std::string_view & data)
  {
    if (data.size() == 0 || closed)
    {
      return true;
    }

    streamBuffer.append(data);

    size_t readLength = 0;
    std::pair<RefCounted<const ::LogOperation>, size_t> result;

    const char * buffer = streamBuffer.data();
    size_t bufferSize = streamBuffer.size();
    while ((result = DeserializeLogOperation(std::string_view(buffer, bufferSize),
      bufferSize)).second > 0)
    {
      writeToDestination(result.first);
      buffer += result.second;
      bufferSize -= result.second;
      readLength += result.second;
    }

    streamBuffer.erase(0, readLength);

    return true;
  }

  template <Subformat F, DeserializeDirection D>
  bool LogOperationDeserializer<F, D>::writeReverse(const std::string_view & data)
  {
    if (data.size() == 0 || closed)
    {
      return true;
    }

    //NOTE: this isn't great
    streamBuffer.insert(0, data);

    size_t readLength = 0;
    std::pair<RefCounted<const ::LogOperation>, size_t> result;

    const char * buffer = streamBuffer.data();
    size_t bufferSize = streamBuffer.size();
    while ((result = DeserializeLogOperation(std::string_view(buffer, bufferSize),
      bufferSize)).second > 0)
    {
      writeToDestination(result.first);
      bufferSize -= result.second;
      readLength += result.second;
    }

    streamBuffer.erase(streamBuffer.size() - readLength, readLength);

    return true;
  }

  template <Subformat F, DeserializeDirection D>
  void LogOperationDeserializer<F, D>::close()
  {
    if (!closed)
    {
      closed = true;
    }
  }

  template <Subformat F, DeserializeDirection D>
  bool LogOperationDeserializer<F, D>::writeToDestination(const RefCounted<const ::LogOperation> & data)
  {
    if (destination == nullptr)
    {
      return false;
    }

    return destination->write(data);
  }

  template <Subformat F, DeserializeDirection D>
  void LogOperationDeserializer<F, D>::pipeTo(IWritableStream<RefCounted<const ::LogOperation>> & writableStream)
  {
    destination = &writableStream;
  }

  template <>
  std::pair<RefCounted<const ::LogOperation>, size_t> LogOperationDeserializer<Subformat::Full, DeserializeDirection::Forward>::DeserializeLogOperation(const std::string_view & data, size_t bufLength)
  {
    if (bufLength < LogOperationFull::getStructSize())
    {
      return std::make_pair(RefCounted<const ::LogOperation>(), 0);
    }

    auto serializedOp = reinterpret_cast<const LogOperationFull *>(data.data());

    size_t opDataSize = serializedOp->getSize();
    if (bufLength < opDataSize)
    {
      return std::make_pair(RefCounted<const ::LogOperation>(), 0);
    }

    ::OperationType opType;
    Deserialize(opType, serializedOp->op.type);

    size_t internalOpSize = ::LogOperation::getSize(opType, serializedOp->op.getDataSize());
    auto op = reinterpret_cast<::LogOperation *>(new uint8_t[internalOpSize]);
    Deserialize(op->ts, serializedOp->ts);
    Deserialize(op->tag, serializedOp->tag);
    Deserialize(op->op, serializedOp->op);

    return std::make_pair(RefCounted<const ::LogOperation>(op), opDataSize);
  }

  template <>
  std::pair<RefCounted<const ::LogOperation>, size_t> LogOperationDeserializer<Subformat::Full, DeserializeDirection::Reverse>::DeserializeLogOperation(const std::string_view & data, size_t bufLength)
  {
    if (bufLength < LogOperationFull::getStructSize())
    {
      return std::make_pair(RefCounted<const ::LogOperation>(), 0);
    }

    uint32_t opDataSize = *reinterpret_cast<const uint32_t *>(data.data() + bufLength - sizeof(uint32_t));
    if (bufLength < opDataSize)
    {
      return std::make_pair(RefCounted<const ::LogOperation>(), 0);
    }

    auto serializedOp = reinterpret_cast<const LogOperationFull *>(data.data() + bufLength - opDataSize);

    ::OperationType opType;
    Deserialize(opType, serializedOp->op.type);

    size_t internalOpSize = ::LogOperation::getSize(opType, serializedOp->op.getDataSize());
    auto op = reinterpret_cast<::LogOperation *>(new uint8_t[internalOpSize]);
    Deserialize(op->ts, serializedOp->ts);
    Deserialize(op->tag, serializedOp->tag);
    Deserialize(op->op, serializedOp->op);

    return std::make_pair(RefCounted<const ::LogOperation>(op), opDataSize);
  }

  template <>
  std::pair<RefCounted<const ::LogOperation>, size_t> LogOperationDeserializer<Subformat::Untagged, DeserializeDirection::Forward>::DeserializeLogOperation(const std::string_view & data, size_t bufLength)
  {
    if (bufLength < LogOperationUntagged::getStructSize())
    {
      return std::make_pair(RefCounted<const ::LogOperation>(), 0);
    }

    auto serializedOp = reinterpret_cast<const LogOperationUntagged *>(data.data());

    size_t opDataSize = serializedOp->getSize();
    if (bufLength < opDataSize)
    {
      return std::make_pair(RefCounted<const ::LogOperation>(), 0);
    }

    ::OperationType opType;
    Deserialize(opType, serializedOp->op.type);

    size_t internalOpSize = ::LogOperation::getSize(opType, serializedOp->op.getDataSize());
    auto op = reinterpret_cast<::LogOperation *>(new uint8_t[internalOpSize]);
    Deserialize(op->ts, serializedOp->ts);
    Deserialize(op->op, serializedOp->op);
    op->tag = ::Tag::Default();

    return std::make_pair(RefCounted<const ::LogOperation>(op), opDataSize);
  }

  template <>
  std::pair<RefCounted<const ::LogOperation>, size_t> LogOperationDeserializer<Subformat::Untagged, DeserializeDirection::Reverse>::DeserializeLogOperation(const std::string_view & data, size_t bufLength)
  {
    if (bufLength < LogOperationUntagged::getStructSize())
    {
      return std::make_pair(RefCounted<const ::LogOperation>(), 0);
    }

    uint32_t opDataSize = *reinterpret_cast<const uint32_t *>(data.data() + bufLength - sizeof(uint32_t));
    if (bufLength < opDataSize)
    {
      return std::make_pair(RefCounted<const ::LogOperation>(), 0);
    }

    auto serializedOp = reinterpret_cast<const LogOperationUntagged *>(data.data() + bufLength - opDataSize);

    ::OperationType opType;
    Deserialize(opType, serializedOp->op.type);

    size_t internalOpSize = ::LogOperation::getSize(opType, serializedOp->op.getDataSize());
    auto op = reinterpret_cast<::LogOperation *>(new uint8_t[internalOpSize]);
    Deserialize(op->ts, serializedOp->ts);
    Deserialize(op->op, serializedOp->op);
    op->tag = ::Tag::Default();

    return std::make_pair(RefCounted<const ::LogOperation>(op), opDataSize);
  }

  template <>
  std::pair<RefCounted<const ::LogOperation>, size_t> LogOperationDeserializer<Subformat::Forward, DeserializeDirection::Forward>::DeserializeLogOperation(const std::string_view & data, size_t bufLength)
  {
    if (bufLength < LogOperationForward::getStructSize())
    {
      return std::make_pair(RefCounted<const ::LogOperation>(), 0);
    }

    auto serializedOp = reinterpret_cast<const LogOperationForward *>(data.data());

    size_t opDataSize = serializedOp->getSize();
    if (bufLength < opDataSize)
    {
      return std::make_pair(RefCounted<const ::LogOperation>(), 0);
    }

    ::OperationType opType;
    Deserialize(opType, serializedOp->op.type);

    size_t internalOpSize = ::LogOperation::getSize(opType, serializedOp->op.getDataSize());
    auto op = reinterpret_cast<::LogOperation *>(new uint8_t[internalOpSize]);
    Deserialize(op->ts, serializedOp->ts);
    Deserialize(op->tag, serializedOp->tag);
    Deserialize(op->op, serializedOp->op);

    return std::make_pair(RefCounted<const ::LogOperation>(op), opDataSize);
  }

  template <>
  std::pair<RefCounted<const ::LogOperation>, size_t> LogOperationDeserializer<Subformat::Type, DeserializeDirection::Forward>::DeserializeLogOperation(const std::string_view & data, size_t bufLength)
  {
    if (bufLength < LogOperationType::getStructSize())
    {
      return std::make_pair(RefCounted<const ::LogOperation>(), 0);
    }

    auto serializedOp = reinterpret_cast<const LogOperationType *>(data.data());

    size_t opDataSize = serializedOp->getSize();
    if (bufLength < opDataSize)
    {
      return std::make_pair(RefCounted<const ::LogOperation>(), 0);
    }

    ::OperationType opType;
    Deserialize(opType, serializedOp->op.type);

    size_t internalOpSize = ::LogOperation::getSize(opType, serializedOp->op.getDataSize());
    auto op = reinterpret_cast<::LogOperation *>(new uint8_t[internalOpSize]);
    op->ts = ::Timestamp::Null;
    op->tag = ::Tag::Default();
    Deserialize(op->op, serializedOp->op);

    return std::make_pair(RefCounted<const ::LogOperation>(op), opDataSize);
  }

  template class LogOperationDeserializer<Subformat::Full, DeserializeDirection::Forward>;
  template class LogOperationDeserializer<Subformat::Full, DeserializeDirection::Reverse>;
  template class LogOperationDeserializer<Subformat::Untagged, DeserializeDirection::Forward>;
  template class LogOperationDeserializer<Subformat::Untagged, DeserializeDirection::Reverse>;
  template class LogOperationDeserializer<Subformat::Forward, DeserializeDirection::Forward>;
  template class LogOperationDeserializer<Subformat::Type, DeserializeDirection::Forward>;
};
