#include "InheritanceContext.h"
#include <stdexcept>

InheritanceContext::InheritanceContext(Timestamp rootId)
{
  parent = nullptr;
  root = rootId;
  offset = std::make_shared<uint32_t>();
  *offset = 0;
  subtreeOffset = 0;
  operationCount = 0;
  attributes = nullptr;
}

InheritanceContext::InheritanceContext(InheritanceContext & inheritanceContext)
{
  parent = &inheritanceContext;
  root = inheritanceContext.root;
  offset = inheritanceContext.offset;
  subtreeOffset = *inheritanceContext.offset;
  operationCount = 0;
  attributes = inheritanceContext.attributes;
}

NodeId InheritanceContext::transformNodeId(const NodeId & nodeId)
{
  //null id should not be transformed
  if (nodeId.isNull())
  {
    return nodeId;
  }

  NodeId newId = NodeId::inheritanceRootFor(root);

  if (nodeId.child != 0)
  {
    uint32_t rootOffset = 0;
    if (nodeId.ts.isTypeRoot())
    {
      rootOffset = subtreeOffset;
    }
    else
    {
      auto it = map.find(nodeId.ts);
      if (it != map.end())
      {
        rootOffset = it->second;
      }
      else
      {
        //this shouldn't happen
        throw std::runtime_error("Timestamp not found in map");
      }
    }

    newId.child = nodeId.child + rootOffset;
    return newId;
  }

  if (nodeId.ts.isTypeRoot())
  {
    //the root node is a special case where the number of nodes
    //should not be incremented
    newId.child = subtreeOffset;
    return newId;
  }

  auto it = map.find(nodeId.ts);
  if (it == map.end())
  {
    newId.child = ++(*offset);
    map[nodeId.ts] = newId.child;
  }
  else
  {
    newId.child = it->second;
  }

  return newId;
}

EdgeId InheritanceContext::transformEdgeId(const EdgeId & edgeId)
{
  return transformNodeId(static_cast<const NodeId &>(edgeId));
}

Timestamp InheritanceContext::transformTimestamp(const Timestamp & timestamp)
{
  return root;
}