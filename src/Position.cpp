#include "Position.h"

Position::Position()
{
  prevEdgeId = EdgeId::Null;
}

Position::Position(const EdgeId & edgeId)
{
  prevEdgeId = edgeId;
}

Position::Position(const uint8_t * data, size_t length)
{
  if (length != sizeof(EdgeId))
  {
    return;
  }

  prevEdgeId = *(reinterpret_cast<const EdgeId *>(data));
}

std::string Position::toString() const
{
  return prevEdgeId.toString();
}