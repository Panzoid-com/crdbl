#pragma once
#include "Edge.h"
#include "ContainerNode.h"
#include "NodeId.h"
#include "Event.h"
#include "Attribute.h"
#include "InheritanceContext.h"
#include "IObjectSerializer.h"
#include <unordered_map>
#include <algorithm>

class ListEdge : public Edge
{
public:
  EdgeId edgeId;
  ListEdge * next = nullptr;
  ListEdge * nextChild = nullptr;

  ListEdge() : next(nullptr), nextChild(nullptr) {}
};

class ListNode : public ContainerNodeImpl<ListEdge>
{
public:
  ListEdge * createEdge(const EdgeId & edgeId, const NodeId & childId, const EdgeId & prevEdgeId, const AttributeMap * attributes, std::function<void(EdgeEvent &)> callback);
  void updateEdgeEffect(const EdgeId & edgeId, int delta, bool deinitialize, std::function<void(EdgeEvent &)> callback);
  void deleteEdge(const EdgeId & edgeId, std::function<void(EdgeEvent &)> callback);
  void initEdge(const EdgeId & edgeId, const NodeId & childId, std::function<void(EdgeEvent &)> callback);

  void serialize(IObjectSerializer & serializer) const;
  void serializeChildren(IObjectSerializer & serializer, bool includePending) const;

// private:
  ListEdge * edgeList = nullptr;
  ListEdge * children = nullptr;

  void addEdge(const EdgeId & edgeId, std::function<void(EdgeEvent &)> callback);
  void removeEdge(const EdgeId & edgeId, std::function<void(EdgeEvent &)> callback);

  void printDbgLists() const;
};