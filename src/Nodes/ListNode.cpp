#include "ListNode.h"
#include <iostream>

ListEdge * ListNode::createEdge(const EdgeId & edgeId, const NodeId & childId,
  const EdgeId & prevEdgeId, const AttributeMap * attributes,
  std::function<void(EdgeEvent &)> callback)
{
  if (attributes == nullptr)
  {
    return nullptr;
  }

  // EdgeId prevEdgeId = getAttributeValueOrDefault<EdgeId>(*attributes, 0);

  ListEdge * edge = getEdge(edgeId);

  if (edge->effect.isInitialized())
  {
    return nullptr;
  }

  edge->edgeId = edgeId;
  edge->childId = childId;
  edge->effect.initialize();

  if (edgeId == prevEdgeId)
  {
    //self reference
    return nullptr;
  }

  ListEdge ** insert;
  if (prevEdgeId.isNull())
  {
    insert = &edgeList;
  }
  else
  {
    ListEdge * prevEdge = getEdge(prevEdgeId);
    insert = &prevEdge->next;
  }

  //insert after newer edges
  while (*insert != nullptr)
  {
    if ((*insert)->edgeId < edgeId || (*insert)->edgeId == edgeId)
    {
      break;
    }

    insert = &(*insert)->next;
  }

  if (edge == *insert)
  {
    //edge is already in the list (and was previously uninitialized)
    if (edge->effect.isVisible())
    {
      addEdge(edgeId, callback);
    }
    return edge;
  }

  ListEdge * nextEdge = *insert;
  *insert = edge;

  //add all edges in this local edge list to the main edge list
  ListEdge * sibling = edge;
  do
  {
    insert = &sibling->next;
    sibling = sibling->next;
  }
  while (sibling != nullptr);

  *insert = nextEdge;

  //add visible edges to the children list
  sibling = edge;
  do
  {
    if (sibling->effect.isVisible())
    {
      addEdge(sibling->edgeId, callback);
    }

    sibling = sibling->next;
  }
  while (sibling != nextEdge);

  return edge;
}

void ListNode::updateEdgeEffect(const EdgeId & edgeId, int delta,
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

void ListNode::deleteEdge(const EdgeId & edgeId, std::function<void(EdgeEvent &)> callback)
{
  removeEdge(edgeId, callback);

  ListEdge * edge = getEdge(edgeId);

  if (edgeList == edge)
  {
    edgeList = edge->next;
  }
  if (children == edge)
  {
    children = edge->nextChild;
  }

  for (auto & e : edges)
  {
    if (e.second->next == edge)
    {
      e.second->next = edge->next;
    }
    if (e.second->nextChild == edge)
    {
      e.second->nextChild = edge->nextChild;
    }
  }

  // ListEdge ** remove = &edgeList;
  // while (*remove != nullptr)
  // {
  //   if (*remove == edge)
  //   {
  //     *remove = edge->next;
  //     break;
  //   }
  //   remove = &(*remove)->next;
  // }

  ContainerNodeImpl<ListEdge>::deleteEdge(edgeId);
}

void ListNode::initEdge(const EdgeId & edgeId, const NodeId & childId, std::function<void(EdgeEvent &)> callback)
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

void ListNode::addEdge(const EdgeId & edgeId, std::function<void(EdgeEvent &)> callback)
{
  ListEdge * edge = getEdge(edgeId);

  size_t index = 0;
  size_t actualIndex = 0;

  ListEdge ** insert = &children;
  ListEdge * currentEdge = edgeList;
  ListEdge * nextChild = children;

  while (currentEdge != nullptr)
  {
    if (currentEdge == edge)
    {
      break;
    }

    if (currentEdge == nextChild)
    {
      if (currentEdge->childId.isPending() == false)
      {
        ++index;
      }
      ++actualIndex;
      insert = &currentEdge->nextChild;
      nextChild = currentEdge->nextChild;
    }

    currentEdge = currentEdge->next;
  }

  if (currentEdge == nullptr)
  {
    //edge is not in the main edge list (dependent item is not yet created)
    return;
  }

  if (currentEdge != nextChild)
  {
    if (currentEdge == *insert)
    {
      //edge is already in the children list
      return;
    }

    //child is not in the children list
    currentEdge->nextChild = *insert;
    *insert = currentEdge;
  }

  //generate added event
  auto event = new NodeAddedEventOrdered();
  event->edgeId = currentEdge->edgeId;
  event->childId = currentEdge->childId;
  event->speculative = currentEdge->childId.isPending();
  event->index = index;
  event->actualIndex = actualIndex;
  callback(*event);
  delete event;

  //NOTE: leaving this here if it is eventually used to preserve state
  //  across multiple calls (which is common, and right now it is inefficient)
  // insert = &currentEdge->nextChild;
  // if (currentEdge->childId.isPending() == false)
  // {
  //   ++index;
  // }
  // ++actualIndex;
}

void ListNode::removeEdge(const EdgeId & edgeId, std::function<void(EdgeEvent &)> callback)
{
  ListEdge * edge = getEdge(edgeId);

  size_t index = 0;
  size_t actualIndex = 0;

  ListEdge * prevEdge = nullptr;
  ListEdge * currentEdge = children;

  while (currentEdge != nullptr)
  {
    if (currentEdge == edge)
    {
      break;
    }

    if (currentEdge->childId.isPending() == false)
    {
      ++index;
    }

    ++actualIndex;
    prevEdge = currentEdge;
    currentEdge = currentEdge->nextChild;
  }

  if (currentEdge == nullptr)
  {
    return;
  }

  if (prevEdge == nullptr)
  {
    children = edge->nextChild;
  }
  else
  {
    prevEdge->nextChild = edge->nextChild;
  }

  edge->nextChild = nullptr;

  //generate removed event
  auto event = new NodeRemovedEventOrdered();
  event->edgeId = edgeId;
  event->childId = edge->childId;
  event->speculative = currentEdge->childId.isPending();
  event->index = index;
  event->actualIndex = actualIndex;
  callback(*event);
  delete event;
}

void ListNode::serialize(IObjectSerializer & serializer) const
{
  Node::serialize(serializer);
  serializer.resumeObject();
  serializer.addPair("childType", childType.getValueOrDefault().toString());
  serializer.endObject();
}

void ListNode::serializeChildren(IObjectSerializer & serializer, bool includePending) const
{
  serializer.startArray();

  for (ListEdge * edge = children; edge != nullptr; edge = edge->nextChild)
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

void ListNode::printDbgLists() const
{
  std::cout << "dbg edge list: ";
  for (ListEdge * edge = edgeList; edge != nullptr; edge = edge->next)
  {
    if (!edge->effect.isVisible())
    {
      std::cout << "(";
    }
    std::cout << edge->edgeId.toString() << ":" << edge->childId.toString();
    if (!edge->effect.isVisible())
    {
      std::cout << ")";
    }

    std::cout << " ";
  }
  std::cout << std::endl;

  std::cout << "dbg children list: ";
  for (ListEdge * edge = children; edge != nullptr; edge = edge->nextChild)
  {
    if (!edge->effect.isVisible())
    {
      std::cout << "(";
    }
    std::cout << edge->edgeId.toString() << ":" << edge->childId.toString();
    if (!edge->effect.isVisible())
    {
      std::cout << ")";
    }

    std::cout << " ";
  }
  std::cout << std::endl;
}