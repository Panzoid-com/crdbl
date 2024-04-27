#pragma once
#include "Edge.h"
#include "ContainerNode.h"
#include "Event.h"
#include "Attribute.h"
#include "InheritanceContext.h"
#include "IObjectSerializer.h"
#include <map>
#include <sstream>
#include <forward_list>

//NOTE: This type is deprecated and will likely be removed in the future.
//  Its use cases are pretty obscure and the one for which it was implemented
//  is probably better off done in a different way.

class OrderedFloat64MapEdge : public Edge
{
public:
  double key;
};

class OrderedFloat64MapNode : public ContainerNodeImpl<OrderedFloat64MapEdge>
{
public:
  OrderedFloat64MapEdge * createEdge(const EdgeId & edgeId, const NodeId & childId, const AttributeMap * attributes, std::function<void(EdgeEvent &)> callback);
  void updateEdgeEffect(const EdgeId & edgeId, int delta, bool deinitialize, std::function<void(EdgeEvent &)> callback);
  void deleteEdge(const EdgeId & edgeId, std::function<void(EdgeEvent &)> callback);
  void initEdge(const EdgeId & edgeId, const NodeId & childId, std::function<void(EdgeEvent &)> callback);

  void serialize(IObjectSerializer & serializer) const;
  void serializeChildren(IObjectSerializer & serializer, bool includePending) const;

// private:
  std::map<double, std::forward_list<EdgeId>> children;

  void addEdge(const EdgeId & edgeId, std::function<void(EdgeEvent &)> callback);
  void removeEdge(const EdgeId & edgeId, std::function<void(EdgeEvent &)> callback);
};