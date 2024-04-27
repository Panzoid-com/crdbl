#include "helpers.h"
#include <OperationLog.h>
#include <stdexcept>

CoreTestWrapper::CoreTestWrapper(std::function<void(const CoreTestWrapper & wrapper, const Event & event)> _eventHandler)
  :
    eventHandler(_eventHandler),
    coreInit(new CoreInit(
      [&](const std::string & type)
      {
        typeSpecsWaiting.push(type);
      },
      [&](const Event & event)
      {
        if (eventHandler)
        {
          eventHandler(*this, event);
        }
      }
    )),
    core(new Core(*coreInit)),
    builder(
      core,
      1
    ),
    builderCallbackStream(new CallbackWritableStream<RefCounted<const LogOperation>>(
      [&](const RefCounted<const LogOperation> & op)
      {
        log.push_back(std::string(reinterpret_cast<const char *>(&(*op)), op->getSize()));
        core->applyOperation(op);
      },
      [&](){})
    )
{
  builder.getReadableStream().pipeTo(*builderCallbackStream);
}

CoreTestWrapper::~CoreTestWrapper()
{
  delete core;
  delete coreInit;
  delete builderCallbackStream;
}

Timestamp CoreTestWrapper::group(std::function<void(OperationBuilder & builder)> applyOps)
{
  builder.startGroup(OperationType::GroupOperation);
  applyOps(builder);
  return builder.commitGroup();
}

void CoreTestWrapper::applyOpsFrom(const CoreTestWrapper & other)
{
  OperationFilter filter;
  //only apply ops we haven't seen
  filter.setClockRange(core->clock, VectorTimestamp());

  for (const auto & it : other.log)
  {
    auto tsOp = reinterpret_cast<const LogOperation *>(it.data());
    if (!filter.filter(*tsOp))
      continue;

    RefCounted<const LogOperation> rc(tsOp);
    core->applyOperation(rc);
    rc.release();
  }
}

std::vector<std::pair<EdgeId, NodeId>> CoreTestWrapper::getSetNodeChildren(NodeId setNodeId, bool includeSpeculative) const
{
  std::vector<std::pair<EdgeId, NodeId>> result;
  auto node = core->getExistingNode(setNodeId);
  if (!node->isDerivedFromType(PrimitiveNodeTypes::Set()))
  {
    throw std::runtime_error("Node is not a set node");
  }
  auto setNode = static_cast<const SetNode *>(node);
  auto edge = setNode->children;
  while (edge)
  {
    if (includeSpeculative || !edge->childId.isPending())
    {
      result.push_back(std::make_pair(edge->edgeId, edge->childId));
    }

    edge = edge->next;
  }
  return result;
}

std::vector<std::pair<EdgeId, NodeId>> CoreTestWrapper::getListNodeChildren(NodeId listNodeId, bool includeSpeculative) const
{
  std::vector<std::pair<EdgeId, NodeId>> result;
  auto node = core->getExistingNode(listNodeId);
  if (!node->isDerivedFromType(PrimitiveNodeTypes::List()))
  {
    throw std::runtime_error("Node is not a list node");
  }
  auto listNode = static_cast<const ListNode *>(node);
  auto edge = listNode->children;
  while (edge)
  {
    if (includeSpeculative || !edge->childId.isPending())
    {
      result.push_back(std::make_pair(edge->edgeId, edge->childId));
    }

    edge = edge->nextChild;
  }
  return result;
}

std::unordered_map<std::string, std::pair<EdgeId, NodeId>> CoreTestWrapper::getMapNodeChildren(NodeId mapNodeId, bool includeSpeculative) const
{
  std::unordered_map<std::string, std::pair<EdgeId, NodeId>> result;
  auto node = core->getExistingNode(mapNodeId);
  if (!node->isDerivedFromType(PrimitiveNodeTypes::Map()))
  {
    throw std::runtime_error("Node is not a map node");
  }
  auto mapNode = static_cast<const MapNode *>(node);
  for (auto & it : mapNode->children)
  {
    for (auto edge : it.second)
    {
      if (!includeSpeculative && edge->childId.isPending())
      {
        continue;
      }

      result[it.first] = std::make_pair(edge->edgeId, edge->childId);
      break;
    }
  }
  return result;
}

std::vector<std::pair<EdgeId, NodeId>> CoreTestWrapper::getReferenceNodeChildren(NodeId mapNodeId, bool includeSpeculative) const
{
  std::vector<std::pair<EdgeId, NodeId>> result;
  auto node = core->getExistingNode(mapNodeId);
  if (!node->isDerivedFromType(PrimitiveNodeTypes::Reference()))
  {
    throw std::runtime_error("Node is not a reference node");
  }
  auto referenceNode = static_cast<const ReferenceNode *>(node);
  auto edge = referenceNode->children;
  for (auto edge = referenceNode->children; edge != nullptr; edge = edge->next)
  {
    if ((!includeSpeculative && edge->childId.isPending()) || !edge->effect.isVisible())
    {
      continue;
    }

    result.push_back(std::make_pair(edge->edgeId, edge->childId));

    break;
  }
  return result;
}

void CoreTestWrapper::resolveTypes()
{
  while (!typeSpecsWaiting.empty())
  {
    std::string type = typeSpecsWaiting.front();
    std::basic_string<char> & spec = types[type];
    core->resolveTypeSpec(type, reinterpret_cast<const Operation *>(spec.data()),
      spec.length());
    typeSpecsWaiting.pop();
  }
}

std::basic_string<char> createTypeSpec(std::function<void(CoreTestWrapper & wrapper)> applyType,
  TypeRepository & typeSpecs)
{
  std::basic_string<char> spec;
  auto typeSpecOutput = [&](const RefCounted<const LogOperation> & data)
  {
    spec.append(reinterpret_cast<const char *>(&data->op), data->op.getSize());
  };

  CoreTestWrapper wrapper;
  wrapper.types = typeSpecs;

  applyType(wrapper);
  wrapper.resolveTypes();

  TypeLogGenerator generator(wrapper.core);
  CallbackWritableStream<RefCounted<const LogOperation>> callbackStream(typeSpecOutput);

  generator.addAllNodes(NodeId::SiteRoot);
  generator.generate(callbackStream);

  return spec;
}

std::basic_string<char> createTypeSpec(std::function<void(OperationBuilder & builder)> applyType,
  TypeRepository & typeSpecs)
{
  return createTypeSpec([&](CoreTestWrapper & wrapper)
  {
    applyType(wrapper.builder);
  }, typeSpecs);
}

std::basic_string<char> createTypeSpecFromRoot(
  std::string rootType,
  std::function<void(const NodeId & rootNodeId, CoreTestWrapper & wrapper)> applyType,
  TypeRepository & typeSpecs)
{
  return createTypeSpec([&](CoreTestWrapper & wrapper)
  {
    NodeId rootNodeId = wrapper.builder.createNode(rootType);
    wrapper.resolveTypes();
    applyType(rootNodeId, wrapper);
  }, typeSpecs);
}

std::basic_string<char> createTypeSpecFromRoot(
  std::string rootType,
  std::function<void(const NodeId & rootNodeId, OperationBuilder & builder)> applyType,
  TypeRepository & typeSpecs)
{
  return createTypeSpec([&](CoreTestWrapper & wrapper)
  {
    NodeId rootNodeId = wrapper.builder.createNode(rootType);
    wrapper.resolveTypes();
    applyType(rootNodeId, wrapper.builder);
  }, typeSpecs);
}

void forEachOp(const OperationLogStorage & log, std::function<void(const std::basic_string<char> &)> callback)
{
  if (log.begin() == log.end())
  {
    return;
  }

  //NOTE: the list may be modified during traversal
  auto beforeEnd = --log.end();
  for (auto it = log.begin(); ; ++it)
  {
    callback(*it);
    if (it == beforeEnd) break;
  }
}

void forEachOpReverse(const OperationLogStorage & log, std::function<void(const std::basic_string<char> &)> callback)
{
  if (log.begin() == log.end())
  {
    return;
  }

  //NOTE: the list may be modified during traversal
  auto beforeEnd = --log.rend();
  for (auto it = log.rbegin(); ; ++it)
  {
    callback(*it);
    if (it == beforeEnd) break;
  }
}

void undoAll(CoreTestWrapper & wrapper, const OperationLogStorage & log)
{
  forEachOp(log, [&](const std::basic_string<char> & op)
  {
    auto tsOp = reinterpret_cast<const LogOperation *>(op.data());

    wrapper.builder.undoOperation(*tsOp);
  });
}

Timestamp undoOperation(CoreTestWrapper & wrapper, const OperationLogStorage & log, const Timestamp & timestamp)
{
  Timestamp result = Timestamp::Null;

  forEachOp(log, [&](const std::basic_string<char> & op)
  {
    auto tsOp = reinterpret_cast<const LogOperation *>(op.data());
    if (tsOp->ts == timestamp)
    {
      result = wrapper.builder.undoOperation(*tsOp);
    }
  });

  return result;
}

void undoOperations(CoreTestWrapper & wrapper, const OperationLogStorage & log, const OperationFilter & filter)
{
  forEachOp(log, [&](const std::basic_string<char> & op)
  {
    auto tsOp = reinterpret_cast<const LogOperation *>(op.data());
    if (filter.filter(*tsOp))
    {
      wrapper.builder.undoOperation(*tsOp);
    }
  });
}

void unapplyAll(CoreTestWrapper & wrapper, const OperationLogStorage & log)
{
  forEachOp(log, [&](const std::basic_string<char> & op)
  {
    auto tsOp = reinterpret_cast<const LogOperation *>(op.data());
    RefCounted<const LogOperation> rc(tsOp);
    wrapper.core->unapplyOperation(rc);
    rc.release();
  });
}

void unapplyOperation(CoreTestWrapper & wrapper, const OperationLogStorage & log, const Timestamp & timestamp)
{
  Timestamp newTs = Timestamp::Null;
  newTs.site = timestamp.site;

  forEachOpReverse(log, [&](const std::basic_string<char> & op)
  {
    auto tsOp = reinterpret_cast<const LogOperation *>(op.data());
    if (tsOp->ts == timestamp)
    {
      RefCounted<const LogOperation> rc(tsOp);
      wrapper.core->unapplyOperation(rc);
      rc.release();
    }
    else if (newTs.clock == 0 && tsOp->ts.site == newTs.site)
    {
      Timestamp effectiveTs = OperationLog::GetFinalTimestamp(*tsOp);
      newTs.clock = effectiveTs.clock;
    }
  });

  wrapper.core->clock.set(newTs);
}

void unapplyOperations(CoreTestWrapper & wrapper, const OperationLogStorage & log, const OperationFilter & filter)
{
  VectorTimestamp newClock = wrapper.core->clock;

  forEachOpReverse(log, [&](const std::basic_string<char> & op)
  {
    auto tsOp = reinterpret_cast<const LogOperation *>(op.data());
    if (filter.filter(*tsOp))
    {
      RefCounted<const LogOperation> rc(tsOp);
      wrapper.core->unapplyOperation(rc);
      rc.release();
      newClock.set(Timestamp(0, tsOp->ts.site));
    }
    else if (newClock.getClockAtSite(tsOp->ts.site) == 0)
    {
      Timestamp effectiveTs = OperationLog::GetFinalTimestamp(*tsOp);
      newClock.set(effectiveTs);
    }
  });

  wrapper.core->clock = newClock;
}

void applyAll(CoreTestWrapper & wrapper, const OperationLogStorage & log)
{
  forEachOp(log, [&](const std::basic_string<char> & op)
  {
    auto tsOp = reinterpret_cast<const LogOperation *>(op.data());
    RefCounted<const LogOperation> rc(tsOp);
    wrapper.core->applyOperation(rc);
    rc.release();
  });
}

void applyOperation(CoreTestWrapper & wrapper, const OperationLogStorage & log, const Timestamp & timestamp)
{
  forEachOp(log, [&](const std::basic_string<char> & op)
  {
    auto tsOp = reinterpret_cast<const LogOperation *>(op.data());
    if (tsOp->ts == timestamp)
    {
      RefCounted<const LogOperation> rc(tsOp);
      wrapper.core->applyOperation(rc);
      rc.release();
    }
  });
}

void applyOperations(CoreTestWrapper & wrapper, const OperationLogStorage & log, const OperationFilter & filter)
{
  forEachOp(log, [&](const std::basic_string<char> & op)
  {
    auto tsOp = reinterpret_cast<const LogOperation *>(op.data());
    if (filter.filter(*tsOp))
    {
      RefCounted<const LogOperation> rc(tsOp);
      wrapper.core->applyOperation(rc);
      rc.release();
    }
  });
}

void filterOperations(CoreTestWrapper & wrapper, const OperationLogStorage & log,
  const OperationFilter & oldFilter, const OperationFilter & newFilter)
{
  for (auto it = log.rbegin(); it != log.rend(); ++it)
  {
    auto op = *it;
    auto tsOp = reinterpret_cast<const LogOperation *>(op.data());
    bool hasOp = oldFilter.filter(*tsOp);
    bool needsOp = newFilter.filter(*tsOp);
    if (hasOp && !needsOp)
    {
      RefCounted<const LogOperation> rc(tsOp);
      wrapper.core->unapplyOperation(rc);
      rc.release();
    }
  }
  auto beforeEnd = --log.end();
  for (auto it = log.begin(); ; ++it)
  {
    auto op = *it;
    auto tsOp = reinterpret_cast<const LogOperation *>(op.data());
    bool hasOp = oldFilter.filter(*tsOp);
    bool needsOp = newFilter.filter(*tsOp);
    if (!hasOp && needsOp)
    {
      RefCounted<const LogOperation> rc(tsOp);
      wrapper.core->applyOperation(rc);
      rc.release();
    }

    if (it == beforeEnd) break;
  }
}