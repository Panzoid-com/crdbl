#pragma once
#include <cstdint>
#include "Timestamp.h"
#include <iostream>

class NodeId
{
public:
  Timestamp ts;
  uint32_t child = 0;

  bool operator<(const NodeId & rhs) const;
  bool operator==(const NodeId & rhs) const = default;
  bool operator!=(const NodeId & rhs) const = default;
  NodeId & operator++();
  bool isNull() const;
  bool isRoot() const;
  bool isPending() const;
  bool isInherited() const;
  NodeId getInheritanceRoot() const;
  std::string toString() const;

  static const NodeId Null;
  static const NodeId Root;
  static const NodeId SiteRoot;
  static const NodeId Pending;

  static inline NodeId inheritanceRootFor(Timestamp ts)
  {
    return { ts, 0 };
  }
};

namespace std
{
  template <>
  struct hash<NodeId>
  {
    std::size_t operator()(const NodeId & id) const
    {
      size_t seed = 0;
      seed ^= hash<Timestamp>()(id.ts) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
      seed ^= hash<uint32_t>()(id.child) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
      return seed;
    }
  };
}