#pragma once
#include "IReadableStream.h"
#include "ReadableStreamBase.h"
#include "IWritableStream.h"
#include "../Core.h"
#include "../LogOperation.h"
#include "../RefCounted.h"
#include "../NodeType.h"
#include <unordered_map>
#include <map>

class TransformOperationStream final :
  public IWritableStream<RefCounted<const LogOperation>>,
  public ReadableStreamBase<RefCounted<const LogOperation>>
{
public:
  TransformOperationStream(bool local = false, const Core * core = nullptr)
    : local(local), core(core) {}

  bool write(const RefCounted<const LogOperation> & data) override;
  void close() override;

  void mapType(std::string fromTypeId, std::string toTypeId);
  void mapTypeNodeId(std::string typeId, uint32_t fromOffset, uint32_t toOffset);
  void mapTypeEdgeId(std::string typeId, uint32_t fromOffset, uint32_t toOffset);

private:
  bool local;
  const Core * core;

  uint32_t globalClockOffset = 0;
  std::unordered_map<std::string, std::string> typeMap;
  std::unordered_map<std::string, std::map<uint32_t, int32_t>> typeOffsetMap;
  std::unordered_map<std::string, std::map<uint32_t, int32_t>> typeEdgeOffsetMap;

  void transformOperation(Operation * operation);
  NodeId transformNodeId(const NodeId & nodeId);
  int32_t getChildOffset(const NodeId & nodeId);
  EdgeId transformEdgeId(const EdgeId & nodeId);
  Timestamp transformTimestamp(const Timestamp & nodeId);
  std::string transformNodeType(const std::string nodeType);
};