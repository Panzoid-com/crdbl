#include "SetNode.h"

SetEdge * SetNode::createEdge(const EdgeId & edgeId, const NodeId & childId,
  const AttributeMap * attributes, std::function<void(EdgeEvent &)> callback)
{
  SetEdge * edge = getEdge(edgeId);

  if (edge->effect.isInitialized())
  {
    return nullptr;
  }

  edge->edgeId = edgeId;
  edge->childId = childId;
  edge->effect.initialize();

  if (edge->effect.isVisible())
  {
    addEdge(edgeId, callback);
  }

  return edge;
}

void SetNode::updateEdgeEffect(const EdgeId & edgeId, int delta, bool deinitialize, std::function<void(EdgeEvent &)> callback)
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

void SetNode::deleteEdge(const EdgeId & edgeId, std::function<void(EdgeEvent &)> callback)
{
  removeEdge(edgeId, callback);
  ContainerNodeImpl<SetEdge>::deleteEdge(edgeId);
}

void SetNode::initEdge(const EdgeId & edgeId, const NodeId & childId, std::function<void(EdgeEvent &)> callback)
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

void SetNode::addEdge(const EdgeId & edgeId, std::function<void(EdgeEvent &)> callback)
{
  SetEdge * edge = getEdge(edgeId);

  bool isEdgeSpeculative = edge->childId.isPending();

  SetEdge ** iter = &children;

  while (*iter != nullptr)
  {
    if ((*iter)->edgeId < edgeId || *iter == edge)
    {
      break;
    }

    iter = &(*iter)->next;
  }

  if (*iter != edge)
  {
    edge->next = *iter;
    *iter = edge;
  }

  auto event = new NodeAddedEvent();
  event->edgeId = edgeId;
  event->childId = edge->childId;
  event->speculative = isEdgeSpeculative;
  callback(*event);
  delete event;
}

void SetNode::removeEdge(const EdgeId & edgeId, std::function<void(EdgeEvent &)> callback)
{
  SetEdge * edge = getEdge(edgeId);

  bool isEdgeSpeculative = edge->childId.isPending();

  SetEdge ** iter = &children;

  while (*iter != nullptr)
  {
    if (*iter == edge)
    {
      break;
    }

    iter = &(*iter)->next;
  }

  if (*iter != edge)
  {
    return;
  }

  *iter = edge->next;
  edge->next = nullptr;

  auto event = new NodeRemovedEvent();
  event->edgeId = edgeId;
  event->childId = edge->childId;
  event->speculative = isEdgeSpeculative;
  callback(*event);
  delete event;
}

void SetNode::serialize(IObjectSerializer & serializer) const
{
  Node::serialize(serializer);
  serializer.resumeObject();
  serializer.addPair("childType", childType.getValueOrDefault().toString());
  serializer.endObject();
}

void SetNode::serializeChildren(IObjectSerializer & serializer, bool includePending) const
{
  serializer.startArray();

  for (SetEdge * edge = children; edge != nullptr; edge = edge->next)
  {
    if (edge->childId.isPending() && !includePending)
    {
      continue;
    }

    serializer.startObject();
    serializer.addPair("edgeId", edge->edgeId);
    serializer.addPair("childId", edge->childId);
    if (edge->childId.isPending())
    {
      serializer.addPair("speculative", true);
    }
    serializer.endObject();
  }

  serializer.endArray();
}