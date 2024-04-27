#include "MapNode.h"

MapEdge * MapNode::createEdge(const EdgeId & edgeId, const NodeId & childId,
  const AttributeMap * attributes, std::function<void(EdgeEvent &)> callback)
{
  if (attributes == nullptr)
  {
    return nullptr;
  }

  std::string key = getAttributeValueOrDefault<std::string>(*attributes, 0);
  if (key.length() == 0)
  {
    return nullptr;
  }

  MapEdge * edge = getEdge(edgeId);
  edge->edgeId = edgeId;
  edge->childId = childId;
  edge->key = key;

  edge->effect.initialize();

  if (edge->effect.isVisible())
  {
    addEdge(edgeId, callback);
  }

  return edge;
}

void MapNode::updateEdgeEffect(const EdgeId & edgeId, int delta, bool deinitialize, std::function<void(EdgeEvent &)> callback)
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

void MapNode::deleteEdge(const EdgeId & edgeId, std::function<void(EdgeEvent &)> callback)
{
  removeEdge(edgeId, callback);
  ContainerNodeImpl<MapEdge>::deleteEdge(edgeId);
}

void MapNode::initEdge(const EdgeId & edgeId, const NodeId & childId, std::function<void(EdgeEvent &)> callback)
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

void MapNode::addEdge(const EdgeId & edgeId, std::function<void(EdgeEvent &)> callback)
{
  MapEdge * edge = getExistingEdge(edgeId);

  bool isEdgeSpeculative = edge->childId.isPending();

  std::forward_list<MapEdge *> & list = children[edge->key];

  auto prevIt = list.before_begin();
  auto it = list.begin();

  bool isFirstVisibleItem = true;
  bool isFirstVisibleSpeculativeItem = true;

  bool generateAddedEvent;

  MapEdge * removedEdge = nullptr;
  MapEdge * removedSpeculativeEdge = nullptr;

  for (; it != list.end(); ++it)
  {
    if ((*it)->edgeId < edgeId || *it == edge)
    {
      break;
    }
    else if (isFirstVisibleItem)
    {
      if ((*it)->effect.isVisible())
      {
        if ((*it)->childId.isPending() == false)
        {
          isFirstVisibleItem = false;
        }

        isFirstVisibleSpeculativeItem = false;
      }
    }

    prevIt = it;
  }

  auto insert = it;

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
    if (it != list.end() && *it == edge)
    {
      it++;
    }

    //find the edge(s) to generate removed events for
    //new edge is speculative: remove pending speculative edge only
    //new edge is not speculative: remove pending speculative edge and
    //  existing non-speculative edge
    while (it != list.end())
    {
      if ((*it)->effect.isVisible())
      {
        if ((*it)->childId.isPending() == false)
        {
          removedEdge = *it;
          break;
        }

        if (removedSpeculativeEdge == nullptr)
        {
          removedSpeculativeEdge = *it;
          if (isEdgeSpeculative)
          {
            break;
          }
        }
      }

      it++;
    }
  }

  if (insert == list.end() || *insert != edge)
  {
    list.insert_after(prevIt, edge);
  }

  if (generateAddedEvent)
  {
    if (removedSpeculativeEdge != nullptr)
    {
      auto event = new NodeRemovedEventMapped();
      event->edgeId = removedSpeculativeEdge->edgeId;
      event->childId = removedSpeculativeEdge->childId;
      //NOTE: alternatively this could specify whether the new incoming edge is
      //  speculative, but instead for now it indicates whether the existing
      //  edge was speculative, which is probably more useful to clients
      event->key = edge->key;
      event->speculative = true;
      callback(*event);
      delete event;
    }

    if (removedEdge != nullptr)
    {
      auto event = new NodeRemovedEventMapped();
      event->edgeId = removedEdge->edgeId;
      event->childId = removedEdge->childId;
      event->key = edge->key;
      event->speculative = isEdgeSpeculative;
      callback(*event);
      delete event;
    }

    auto event = new NodeAddedEventMapped();
    event->edgeId = edgeId;
    event->childId = edge->childId;
    event->speculative = isEdgeSpeculative;
    event->key = edge->key;
    callback(*event);
    delete event;
  }
}

void MapNode::removeEdge(const EdgeId & edgeId, std::function<void(EdgeEvent &)> callback)
{
  MapEdge * edge = getExistingEdge(edgeId);

  bool isEdgeSpeculative = edge->childId.isPending();

  auto mapIt = children.find(edge->key);
  if (mapIt == children.end())
  {
    return;
  }

  std::forward_list<MapEdge *> & list = mapIt->second;

  auto prevIt = list.before_begin();
  auto it = list.begin();

  bool isFirstVisibleItem = true;
  bool isFirstVisibleSpeculativeItem = true;

  bool generateRemovedEvent;

  MapEdge * addedEdge = nullptr;
  MapEdge * addedSpeculativeEdge = nullptr;

  for (; it != list.end(); ++it)
  {
    if (*it == edge)
    {
      break;
    }
    else if (isFirstVisibleItem)
    {
      if ((*it)->effect.isVisible())
      {
        if ((*it)->childId.isPending() == false)
        {
          isFirstVisibleItem = false;
        }

        isFirstVisibleSpeculativeItem = false;
      }
    }

    prevIt = it;
  }

  if (it == list.end())
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

  if (generateRemovedEvent && it != list.end())
  {
    it++;

    //find the edge(s) to generate added events for
    //deleted edge is speculative: re-add newest visible edge only
    //deleted edge is not speculative: re-add newest visible speculative edge
    //  and newest visible non-speculative edge
    while (it != list.end())
    {
      if ((*it)->effect.isVisible())
      {
        if ((*it)->childId.isPending() == false)
        {
          addedEdge = *it;
          break;
        }

        if (addedSpeculativeEdge == nullptr)
        {
          addedSpeculativeEdge = *it;
          if (isEdgeSpeculative)
          {
            break;
          }
        }
      }

      it++;
    }
  }

  list.erase_after(prevIt);

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
    event->speculative = isEdgeSpeculative;
    event->key = edge->key;
    callback(*event);
    delete event;

    if (addedSpeculativeEdge != nullptr)
    {
      auto event = new NodeAddedEventMapped();
      event->edgeId = addedSpeculativeEdge->edgeId;
      event->childId = addedSpeculativeEdge->childId;
      //NOTE: alternatively this could specify whether the deleted edge is
      //  speculative, but instead for now it indicates whether the existing
      //  edge was speculative, which is probably more useful to clients
      event->speculative = true;
      event->key = edge->key;
      callback(*event);
      delete event;
    }

    if (addedEdge != nullptr)
    {
      auto event = new NodeAddedEventMapped();
      event->edgeId = addedEdge->edgeId;
      event->childId = addedEdge->childId;
      event->key = edge->key;
      event->speculative = isEdgeSpeculative;
      callback(*event);
      delete event;
    }
  }
}

void MapNode::serialize(IObjectSerializer & serializer) const
{
  Node::serialize(serializer);
  serializer.resumeObject();
  serializer.addPair("childType", childType.getValueOrDefault().toString());
  serializer.endObject();
}

void MapNode::serializeChildren(IObjectSerializer & serializer, bool includePending) const
{
  serializer.startArray();

  for (auto it = children.begin(); it != children.end(); ++it)
  {
    for (auto edge : it->second)
    {
      bool pending = edge->childId.isPending();
      if (pending && !includePending)
      {
        continue;
      }

      serializer.startObject();
      serializer.addPair("edgeId", edge->edgeId);
      serializer.addPair("childId", edge->childId);
      serializer.addPair("key", it->first);
      if (pending)
      {
        serializer.addPair("speculative", true);
      }
      serializer.endObject();

      if (!pending)
      {
        break;
      }
    }
  }

  serializer.endArray();
}