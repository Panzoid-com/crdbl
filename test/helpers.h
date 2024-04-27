#pragma once
#include <queue>
#include <list>
#include <unordered_map>
#include <iomanip>
#include <functional>
#include <Core.h>
#include <NodeType.h>
#include <OperationFilter.h>
#include <TypeLogGenerator.h>
#include <OperationBuilder.h>
#include <Streams/CallbackWritableStream.h>
#include <RefCounted.h>

using TypeRepository = std::unordered_map<std::string, std::basic_string<char>>;
using OperationLogStorage = std::list<std::basic_string<char>>;
using EventList = std::vector<Event>;

struct CoreTestWrapper
{
  CoreTestWrapper(std::function<void(const CoreTestWrapper & wrapper, const Event & event)> eventHandler = nullptr);
  ~CoreTestWrapper();

  Timestamp group(std::function<void(OperationBuilder & builder)> applyOps);

  template <typename T>
  T getNodeValue(NodeId nodeId) const
  {
    return static_cast<const ValueNode<T> *>(core->getExistingNode(nodeId))->value.getValue();
  }
  std::string getNodeBlockValue(NodeId nodeId) const
  {
    return static_cast<const BlockValueNode<char> *>(core->getExistingNode(nodeId))->value.toString();
  }

  void applyOpsFrom(const CoreTestWrapper & other);

  std::vector<std::pair<EdgeId, NodeId>> getSetNodeChildren(NodeId listNodeId, bool includeSpeculative = false) const;
  std::vector<std::pair<EdgeId, NodeId>> getListNodeChildren(NodeId listNodeId, bool includeSpeculative = false) const;
  std::unordered_map<std::string, std::pair<EdgeId, NodeId>> getMapNodeChildren(NodeId mapNodeId, bool includeSpeculative = false) const;
  std::vector<std::pair<EdgeId, NodeId>> getReferenceNodeChildren(NodeId mapNodeId, bool includeSpeculative = false) const;

  void resolveTypes();

  std::function<void(const CoreTestWrapper & wrapper, const Event & event)> eventHandler;

  CoreInit * coreInit;
  Core * core;

  CallbackWritableStream<RefCounted<const LogOperation>> * builderCallbackStream;

  OperationBuilder builder;
  OperationLogStorage log;
  TypeRepository types;
  EventList events;
  std::queue<std::string> typeSpecsWaiting;
};

std::basic_string<char> createTypeSpec(std::function<void(OperationBuilder & builder)> applyType,
  TypeRepository & typeSpecs);
std::basic_string<char> createTypeSpec(std::function<void(CoreTestWrapper & wrapper)> applyType,
  TypeRepository & typeSpecs);
std::basic_string<char> createTypeSpecFromRoot(std::string rootType,
  std::function<void(const NodeId & rootNodeId, CoreTestWrapper & wrapper)> applyType,
  TypeRepository & typeSpecs);
std::basic_string<char> createTypeSpecFromRoot(std::string rootType,
  std::function<void(const NodeId & rootNodeId, OperationBuilder & builder)> applyType,
  TypeRepository & typeSpecs);

void forEachOp(const OperationLogStorage & log, std::function<void(const std::basic_string<char> &)> callback);

void undoAll(CoreTestWrapper & wrapper, const OperationLogStorage & log);
Timestamp undoOperation(CoreTestWrapper & wrapper, const OperationLogStorage & log, const Timestamp & timestamp);
void undoOperations(CoreTestWrapper & wrapper, const OperationLogStorage & log, const OperationFilter & filter);
void unapplyAll(CoreTestWrapper & wrapper, const OperationLogStorage & log);
void unapplyOperation(CoreTestWrapper & wrapper, const OperationLogStorage & log, const Timestamp & timestamp);
void unapplyOperations(CoreTestWrapper & wrapper, const OperationLogStorage & log, const OperationFilter & filter);
void applyAll(CoreTestWrapper & wrapper, const OperationLogStorage & log);
void applyOperation(CoreTestWrapper & wrapper, const OperationLogStorage & log, const Timestamp & timestamp);
void applyOperations(CoreTestWrapper & wrapper, const OperationLogStorage & log, const OperationFilter & filter);

void filterOperations(CoreTestWrapper & wrapper, const OperationLogStorage & log,
  const OperationFilter & oldFilter, const OperationFilter & newFilter);