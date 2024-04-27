#pragma once
#include "Node.h"
#include "EdgeId.h"
#include <unordered_map>

class ContainerNode : public Node
{
public:
  enum AttributeType : AttributeId
  {
    ChildType = 0
  };

  Attribute<NodeType> childType;
};

template <class T>
class ContainerNodeImpl : public ContainerNode
{
public:
  ~ContainerNodeImpl();
  T * getEdge(const EdgeId & edgeId);
  T * getExistingEdge(const EdgeId & edgeId) const;
  void deleteEdge(const EdgeId & edgeId);
  std::unordered_map<EdgeId, T *> edges;
};