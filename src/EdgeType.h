#pragma once
#include <cstdint>

enum class EdgeType : uint8_t
{
  InsertAfter,
  InsertBefore,

  Key,

  AbsolutePosition
};