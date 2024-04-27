#pragma once
#include "NodeType.h"
#include "PrimitiveNodeTypes.h"
#include "Effect.h"
#include "Attribute.h"
#include "IObjectSerializer.h"
#include <string>
#include <cstdint>
#include <vector>
#include <algorithm>

class Node
{
public:
  std::vector<NodeType> type;
  uint32_t createdByRootOffset;

  Effect effect;

  bool isAbstractType() const;
  void addType(NodeType newType);
  NodeType getType() const;
  NodeType getBaseType() const;
  bool isDerivedFromType(NodeType type) const;

  void serialize(IObjectSerializer & serializer) const;
};