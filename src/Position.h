#pragma once
#include "EdgeId.h"
#include <string>

class Position
{
public:
  EdgeId prevEdgeId;

  Position();
  Position(const EdgeId & edgeId);
  Position(const uint8_t * data, size_t length);

  std::string toString() const;
};
