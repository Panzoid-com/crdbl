#pragma once
#include <NodeId.h>
#include <EdgeId.h>

using StringNodeId = std::string;
using StringEdgeId = std::string;

inline NodeId stringToNodeId(const StringNodeId & stringNodeId)
{
  NodeId nodeId;

  size_t startPos = 0, endPos;
  char delimiter = ',';
  const char* strStart = stringNodeId.c_str();
  char* strEnd;

  // Parsing clock
  endPos = stringNodeId.find(delimiter, startPos);
  if (endPos == std::string::npos)
  {
    return NodeId::Null;
  }
  long clockVal = std::strtol(strStart, &strEnd, 10);
  if (strEnd == strStart || strEnd != strStart + endPos || clockVal < INT_MIN || clockVal > INT_MAX)
  {
    return NodeId::Null;
  }
  nodeId.ts.clock = static_cast<int>(clockVal);
  startPos = endPos + 1;
  strStart = stringNodeId.c_str() + startPos;

  // Parsing site
  endPos = stringNodeId.find(delimiter, startPos);
  if (endPos == std::string::npos)
  {
    return NodeId::Null;
  }
  long siteVal = std::strtol(strStart, &strEnd, 10);
  if (strEnd == strStart || strEnd != strStart + (endPos - startPos) || siteVal < INT_MIN || siteVal > INT_MAX)
  {
    return NodeId::Null;
  }
  nodeId.ts.site = static_cast<int>(siteVal);
  startPos = endPos + 1;
  strStart = stringNodeId.c_str() + startPos;

  // Parsing child
  long childVal = std::strtol(strStart, &strEnd, 10);
  if (strEnd == strStart || *strEnd != '\0' || childVal < INT_MIN || childVal > INT_MAX)
  {
    return NodeId::Null;
  }
  nodeId.child = static_cast<int>(childVal);

  return nodeId;
}