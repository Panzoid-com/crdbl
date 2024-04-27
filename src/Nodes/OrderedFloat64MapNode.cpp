#include "OrderedFloat64MapNode.h"
#include "Json.h"

OrderedFloat64MapEdge * OrderedFloat64MapNode::createEdge(const EdgeId & edgeId, const NodeId & childId,
  const AttributeMap * attributes, std::function<void(EdgeEvent &)> callback)
{
  if (attributes == nullptr)
  {
    return nullptr;
  }

  bool success;
  double key = getAttributeValue<double>(*attributes, 0, success);
  if (success == false)
  {
    return nullptr;
  }

  OrderedFloat64MapEdge * edge = getEdge(edgeId);
  edge->childId = childId;
  edge->key = key;

  edge->effect.initialize();

  if (edge->effect.isVisible())
  {
    addEdge(edgeId, callback);
  }

  return edge;
}

void OrderedFloat64MapNode::updateEdgeEffect(const EdgeId & edgeId, int delta, bool deinitialize, std::function<void(EdgeEvent &)> callback)
{
  Edge * edge = getEdge(edgeId);

  bool prevState = edge->effect.isVisible();

  if (deinitialize)
  {
    edge->effect.deinitialize();
  }
  else
  {
    edge->effect += delta;
  }

  bool newState = edge->effect.isVisible();

  if (newState == prevState)
  {
    return;
  }

  if (newState)
  {
    addEdge(edgeId, callback);
  }
  else
  {
    removeEdge(edgeId, callback);
  }
}

void OrderedFloat64MapNode::deleteEdge(const EdgeId & edgeId, std::function<void(EdgeEvent &)> callback)
{
  removeEdge(edgeId, callback);
  ContainerNodeImpl<OrderedFloat64MapEdge>::deleteEdge(edgeId);
}

void OrderedFloat64MapNode::initEdge(const EdgeId & edgeId, const NodeId & childId, std::function<void(EdgeEvent &)> callback)
{
  Edge * edge = getEdge(edgeId);
  edge->childId = childId;

  if (edge->effect.isVisible() == false)
  {
    removeEdge(edgeId, callback);
  }
  else
  {
    addEdge(edgeId, callback);
  }
}

void OrderedFloat64MapNode::addEdge(const EdgeId & edgeId, std::function<void(EdgeEvent &)> callback)
{
  OrderedFloat64MapEdge * edge = getExistingEdge(edgeId);

  std::forward_list<EdgeId> & list = children[edge->key];

  auto prevIt = list.before_begin();
  auto it = list.begin();

  for (; it != list.end(); ++it)
  {
    if (*it < edgeId || *it == edgeId)
    {
      break;
    }

    prevIt = it;
  }

  if (it == list.end() || *it != edgeId)
  {
    list.insert_after(prevIt, edgeId);
  }

  if (prevIt == list.before_begin())
  {
    if (it != list.end() && *it != edgeId)
    {
      OrderedFloat64MapEdge * oldEdge = getExistingEdge(*it);
      auto event = new NodeRemovedEventMapped();
      event->edgeId = *it;
      event->childId = oldEdge->childId;
      event->speculative = oldEdge->childId.isPending();
      event->key = DoubleToString(oldEdge->key);
      callback(*event);
      delete event;
    }

    auto event = new NodeAddedEventMapped();
    event->edgeId = edgeId;
    event->childId = edge->childId;
    event->speculative = edge->childId.isPending();
    event->key = DoubleToString(edge->key);
    callback(*event);
    delete event;
  }
}

void OrderedFloat64MapNode::removeEdge(const EdgeId & edgeId, std::function<void(EdgeEvent &)> callback)
{
  OrderedFloat64MapEdge * edge = getExistingEdge(edgeId);

  auto mapIt = children.find(edge->key);
  if (mapIt == children.end())
  {
    return;
  }

  std::forward_list<EdgeId> & list = mapIt->second;

  auto prevIt = list.before_begin();
  auto it = list.begin();

  for (; it != list.end(); ++it)
  {
    if (*it == edgeId)
    {
      it = list.erase_after(prevIt);
      break;
    }

    prevIt = it;
  }

  bool generateRemovedEvent = (prevIt == list.before_begin());
  bool generateAddedEvent = (it != list.end());

  //remove key if there are no children
  if (list.begin() == list.end())
  {
    children.erase(mapIt);
  }

  if (generateRemovedEvent)
  {
    auto event = new NodeRemovedEventMapped();
    event->edgeId = edgeId;
    event->childId = edge->childId;
    event->speculative = edge->childId.isPending();
    event->key = DoubleToString(edge->key);
    callback(*event);
    delete event;

    if (generateAddedEvent)
    {
      OrderedFloat64MapEdge * newEdge = getExistingEdge(*it);
      auto event = new NodeAddedEventMapped();
      event->edgeId = *it;
      event->childId = newEdge->childId;
      event->speculative = newEdge->childId.isPending();
      event->key = DoubleToString(newEdge->key);
      callback(*event);
      delete event;
    }
  }
}

void OrderedFloat64MapNode::serialize(IObjectSerializer & serializer) const
{
  Node::serialize(serializer);

  serializer.resumeObject();
  serializer.addPair("childType", childType.getValueOrDefault().toString());
  serializer.endObject();
}

void OrderedFloat64MapNode::serializeChildren(IObjectSerializer & serializer, bool includePending) const
{
  serializer.startArray();

  for (auto it = children.begin(); it != children.end(); ++it)
  {
    EdgeId edgeId = it->second.front();
    Edge * edge = getExistingEdge(edgeId);

    if (edge->childId.isPending() && includePending == false)
    {
      continue;
    }

    serializer.startObject();
    serializer.addPair("edgeId", edgeId);
    serializer.addPair("childId", edge->childId);
    serializer.addPair("key", it->first);
    if (edge->childId.isPending())
    {
      serializer.addPair("speculative", true);
    }
    serializer.endObject();
  }

  serializer.endArray();
}