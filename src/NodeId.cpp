#include "NodeId.h"

bool NodeId::operator<(const NodeId & rhs) const
{
  return ts < rhs.ts || (ts == rhs.ts && child < rhs.child);
}

NodeId & NodeId::operator++()
{
  child++;
  return *this;
}

bool NodeId::isNull() const
{
  return *this == Null;
}

bool NodeId::isRoot() const
{
  return *this == Root;
}

bool NodeId::isPending() const
{
  return *this == Pending;
}

bool NodeId::isInherited() const
{
  return child != 0;
}

NodeId NodeId::getInheritanceRoot() const
{
  return { ts, 0 };
}

std::string NodeId::toString() const
{
  std::string outString;

  outString += std::to_string(ts.clock);
  outString += ",";
  outString += std::to_string(ts.site);
  outString += ",";
  outString += std::to_string(child);

  return outString;
}

inline const NodeId NodeId::Null = {{0, 0}, 0};
inline const NodeId NodeId::Root = {{0, 0}, 1};
inline const NodeId NodeId::SiteRoot = {{1, 1}, 0};
inline const NodeId NodeId::Pending = {{0, 0}, 2};