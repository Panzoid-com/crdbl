#pragma once
#include "NodeId.h"
#include "Effect.h"
#include "EdgeType.h"
#include "EdgeId.h"

class Edge
{
public:
  NodeId childId;
  Effect effect;
  uint32_t createdByRootOffset;
};

//TODO: below are unused; use or delete

struct EdgeData
{
  EdgeType type;

  size_t getStructSize() const;
  size_t getSize() const;
};

struct InsertAfterEdgeData : public EdgeData
{
  EdgeId prevEdge;
};

struct InsertBeforeEdgeData : public EdgeData
{
  EdgeId nextEdge;
};

struct KeyEdgeData : public EdgeData
{
  uint32_t length;
  uint8_t key[];
};

struct AbsolutePositionEdgeData : public EdgeData
{
  double position;
};