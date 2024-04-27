#pragma once
#include "Edge.h"
#include "ContainerNode.h"
#include "NodeId.h"
#include "Event.h"
#include "Attribute.h"
#include "InheritanceContext.h"
#include "IObjectSerializer.h"
#include <algorithm>

class ReferenceEdge : public Edge
{
public:
  EdgeId edgeId;
  ReferenceEdge * next = nullptr;

  ReferenceEdge() : next(nullptr) {}
};

class ReferenceNode : public ContainerNodeImpl<ReferenceEdge>
{
public:
  bool nullable = true;

  ReferenceEdge * createEdge(const EdgeId & edgeId, const NodeId & childId, const AttributeMap * attributes, std::function<void(EdgeEvent &)> callback);
  void updateEdgeEffect(const EdgeId & edgeId, int delta, bool deinitialize, std::function<void(EdgeEvent &)> callback);
  void deleteEdge(const EdgeId & edgeId, std::function<void(EdgeEvent &)> callback);
  void initEdge(const EdgeId & edgeId, const NodeId & childId, std::function<void(EdgeEvent &)> callback);

  void serialize(IObjectSerializer & serializer) const;
  void serializeChildren(IObjectSerializer & serializer, bool includePending) const;

// private:
  ReferenceEdge * children = nullptr;

  void addEdge(const EdgeId & edgeId, std::function<void(EdgeEvent &)> callback);
  void removeEdge(const EdgeId & edgeId, std::function<void(EdgeEvent &)> callback);
};