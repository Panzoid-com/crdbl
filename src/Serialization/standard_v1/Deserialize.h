#pragma once
#include "LogOperation.h"
#include "../../LogOperation.h"

namespace Serialization_standard_v1
{
  void Deserialize(uint8_t * dst, const uint8_t * src, size_t length);

  template <class Dst, class Src>
  void Deserialize(Dst & dst, const Src & src);
};