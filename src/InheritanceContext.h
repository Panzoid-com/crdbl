#pragma once
#include "Timestamp.h"
#include "Attribute.h"
#include "NodeId.h"
#include "EdgeId.h"
#include "Promise.h"
#include <unordered_map>
#include <memory>

class InheritanceContext
{
public:
  InheritanceContext * parent;
  Timestamp root;
  std::shared_ptr<uint32_t> offset;
  uint32_t subtreeOffset;
  std::unordered_map<Timestamp, uint32_t> map;

  //used when applying ops to replace explicit timestamps
  uint32_t operationCount;

  NodeType type;
  AttributeMap * attributes;

  Promise<void> callback;

  InheritanceContext(Timestamp rootId);
  InheritanceContext(InheritanceContext & idTransform);
  NodeId transformNodeId(const NodeId & nodeId);
  EdgeId transformEdgeId(const EdgeId & edgeId);
  Timestamp transformTimestamp(const Timestamp & timestamp);
};