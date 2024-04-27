#pragma once
#include "Core.h"
#include <unordered_map>
#include "NodeId.h"
#include "Streams/IWritableStream.h"
#include "RefCounted.h"

class TypeLogGenerator
{
public:
  using OutputFn = std::function<void(const char * data, size_t length)>;
  using FilterFn = std::function<bool(const NodeId & nodeId)>;

  TypeLogGenerator(const Core * core);

  void addNode(const NodeId & nodeId);
  void addAllNodes(const NodeId & rootId);
  void addAllNodesWithFilter(const NodeId & rootId, FilterFn filterFn);
  void generate(IWritableStream<RefCounted<const LogOperation>> & logStream);

private:
  static RefCounted<LogOperation> getOpBuffer(size_t opSize);

  template <class T>
  static void findAllNodes(const Core * core, const NodeId & rootId,
    std::unordered_map<NodeId, NodeId> & nodeMap, T filterFn);
  static NodeId generateTypeLog(const Core * core,
    std::unordered_map<NodeId, NodeId> & addedNodes, const NodeId & nodeId,
    Timestamp & ts, IWritableStream<RefCounted<const LogOperation>> & logStream);

  const Core * core;
  NodeId rootId = NodeId::Null;
  Timestamp ts = Timestamp::Null;
  std::unordered_map<NodeId, NodeId> nodeMap;
};