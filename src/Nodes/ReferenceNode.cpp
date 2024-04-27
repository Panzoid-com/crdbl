#include "ReferenceNode.h"

ReferenceEdge * ReferenceNode::createEdge(const EdgeId & edgeId, const NodeId & childId,
  const AttributeMap * attributes, std::function<void(EdgeEvent &)> callback)
{
  ReferenceEdge * edge = getEdge(edgeId);
  edge->edgeId = edgeId;
  edge->childId = childId;

  edge->effect.initialize();

  if (edge->effect.isVisible())
  {
    addEdge(edgeId, callback);
  }

  return edge;
}

void ReferenceNode::updateEdgeEffect(const EdgeId & edgeId, int delta,
  bool deinitialize, std::function<void(EdgeEvent &)> callback)
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

void ReferenceNode::deleteEdge(const EdgeId & edgeId, std::function<void(EdgeEvent &)> callback)
{
  removeEdge(edgeId, callback);
  ContainerNodeImpl<ReferenceEdge>::deleteEdge(edgeId);
}

void ReferenceNode::initEdge(const EdgeId & edgeId, const NodeId & childId, std::function<void(EdgeEvent &)> callback)
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

void ReferenceNode::addEdge(const EdgeId & edgeId, std::function<void(EdgeEvent &)> callback)
{
  ReferenceEdge * edge = getExistingEdge(edgeId);

  bool isEdgeSpeculative = edge->childId.isPending();

  ReferenceEdge ** insert = nullptr;
  ReferenceEdge ** iter = &children;

  bool isFirstVisibleItem = true;
  bool isFirstVisibleSpeculativeItem = true;

  bool generateAddedEvent;

  ReferenceEdge * removedEdge = nullptr;
  ReferenceEdge * removedSpeculativeEdge = nullptr;

  //find insert location
  while (*iter != nullptr)
  {
    if ((*iter)->edgeId < edgeId || *iter == edge)
    {
      break;
    }
    else if (isFirstVisibleItem)
    {
      if ((*iter)->effect.isVisible())
      {
        if ((*iter)->childId.isPending() == false)
        {
          isFirstVisibleItem = false;
        }

        isFirstVisibleSpeculativeItem = false;
      }
    }

    iter = &(*iter)->next;
  }

  insert = iter;

  if (isEdgeSpeculative)
  {
    generateAddedEvent = isFirstVisibleSpeculativeItem;
  }
  else
  {
    generateAddedEvent = isFirstVisibleItem;
  }

  if (generateAddedEvent)
  {
    if (*iter == edge)
    {
      iter = &(*iter)->next;
    }

    //find the edge(s) to generate removed events for
    //new edge is speculative: remove pending speculative edge only
    //new edge is not speculative: remove pending speculative edge and
    //  existing non-speculative edge
    while (*iter != nullptr)
    {
      if ((*iter)->effect.isVisible())
      {
        if ((*iter)->childId.isPending() == false)
        {
          removedEdge = *iter;
          break;
        }

        if (removedSpeculativeEdge == nullptr)
        {
          removedSpeculativeEdge = *iter;
          if (isEdgeSpeculative)
          {
            break;
          }
        }
      }

      iter = &(*iter)->next;
    }
  }

  if (*insert != edge)
  {
    edge->next = *insert;
    *insert = edge;
  }

  if (generateAddedEvent)
  {
    if (removedSpeculativeEdge != nullptr)
    {
      auto event = new NodeRemovedEvent();
      event->edgeId = removedSpeculativeEdge->edgeId;
      event->childId = removedSpeculativeEdge->childId;
      //NOTE: alternatively this could specify whether the new incoming edge is
      //  speculative, but instead for now it indicates whether the existing
      //  edge was speculative, which is probably more useful to clients
      event->speculative = true;
      callback(*event);
      delete event;
    }

    //always generate at least one removed event when the value changes, since
    //  references are always considered to have some value (even if it is null)
    if (removedSpeculativeEdge == nullptr || isEdgeSpeculative == false)
    {
      auto event = new NodeRemovedEvent();
      if (removedEdge != nullptr)
      {
        event->edgeId = removedEdge->edgeId;
        event->childId = removedEdge->childId;
      }
      else
      {
        event->edgeId = EdgeId::Null;
        event->childId = EdgeId::Null;
      }
      event->speculative = isEdgeSpeculative;
      callback(*event);
      delete event;
    }

    auto event = new NodeAddedEvent();
    event->edgeId = edgeId;
    event->childId = edge->childId;
    event->speculative = isEdgeSpeculative;
    callback(*event);
    delete event;
  }
}

void ReferenceNode::removeEdge(const EdgeId & edgeId, std::function<void(EdgeEvent &)> callback)
{
  ReferenceEdge * edge = getExistingEdge(edgeId);

  bool isEdgeSpeculative = edge->childId.isPending();

  ReferenceEdge ** remove = nullptr;
  ReferenceEdge ** iter = &children;

  bool isFirstVisibleItem = true;
  bool isFirstVisibleSpeculativeItem = true;

  bool generateRemovedEvent;

  ReferenceEdge * addedEdge = nullptr;
  ReferenceEdge * addedSpeculativeEdge = nullptr;

  //find remove location
  while (*iter != nullptr)
  {
    if (*iter == edge)
    {
      break;
    }
    else if (isFirstVisibleItem)
    {
      if ((*iter)->effect.isVisible())
      {
        if ((*iter)->childId.isPending() == false)
        {
          isFirstVisibleItem = false;
        }

        isFirstVisibleSpeculativeItem = false;
      }
    }

    iter = &(*iter)->next;
  }

  remove = iter;

  if (*remove != edge)
  {
    return;
  }

  if (isEdgeSpeculative)
  {
    generateRemovedEvent = isFirstVisibleSpeculativeItem;
  }
  else
  {
    generateRemovedEvent = isFirstVisibleItem;
  }

  if (generateRemovedEvent && *iter != nullptr)
  {
    iter = &(*iter)->next;

    //find the edge(s) to generate added events for
    //deleted edge is speculative: re-add newest visible edge only
    //deleted edge is not speculative: re-add newest visible speculative edge
    //  and newest visible non-speculative edge
    while (*iter != nullptr)
    {
      if ((*iter)->effect.isVisible())
      {
        if ((*iter)->childId.isPending() == false)
        {
          addedEdge = *iter;
          break;
        }

        if (addedSpeculativeEdge == nullptr)
        {
          addedSpeculativeEdge = *iter;
          if (isEdgeSpeculative)
          {
            break;
          }
        }
      }

      iter = &(*iter)->next;
    }
  }

  *remove = edge->next;
  edge->next = nullptr;

  if (generateRemovedEvent)
  {
    auto event = new NodeRemovedEvent();
    event->edgeId = edgeId;
    event->childId = edge->childId;
    event->speculative = isEdgeSpeculative;
    callback(*event);
    delete event;

    if (addedSpeculativeEdge != nullptr)
    {
      auto event = new NodeAddedEvent();
      event->edgeId = addedSpeculativeEdge->edgeId;
      event->childId = addedSpeculativeEdge->childId;
      //NOTE: alternatively this could specify whether the deleted edge is
      //  speculative, but instead for now it indicates whether the existing
      //  edge was speculative, which is probably more useful to clients
      event->speculative = true;
      callback(*event);
      delete event;
    }

    //always generate at least one added event when the value changes, since
    //  references are always considered to have some value (even if it is null)
    if (addedSpeculativeEdge == nullptr || isEdgeSpeculative == false)
    {
      auto event = new NodeAddedEvent();
      if (addedEdge != nullptr)
      {
        event->edgeId = addedEdge->edgeId;
        event->childId = addedEdge->childId;
      }
      else
      {
        event->edgeId = EdgeId::Null;
        event->childId = EdgeId::Null;
      }
      event->speculative = isEdgeSpeculative;
      callback(*event);
      delete event;
    }
  }
}

void ReferenceNode::serialize(IObjectSerializer & serializer) const
{
  Node::serialize(serializer);

  serializer.resumeObject();
  serializer.addPair("childType", childType.getValueOrDefault().toString());
  serializer.addPair("nullable", nullable);
  serializer.endObject();
}

void ReferenceNode::serializeChildren(IObjectSerializer & serializer, bool includePending) const
{
  serializer.startArray();

  for (ReferenceEdge * edge = children; edge != nullptr; edge = edge->next)
  {
    if (!edge->effect.isVisible())
      continue;

    if (!includePending && edge->childId.isPending())
      continue;

    serializer.startObject();

    serializer.addPair("edgeId", edge->edgeId);
    serializer.addPair("childId", edge->childId);
    if (edge->childId.isPending())
    {
      serializer.addPair("speculative", true);
      serializer.endObject();
    }
    else
    {
      serializer.endObject();
      serializer.endArray();
      return;
    }
  }

  serializer.startObject();
  serializer.addPair("edgeId", EdgeId::Null);
  serializer.addPair("childId", NodeId::Null);
  serializer.endObject();

  serializer.endArray();
}