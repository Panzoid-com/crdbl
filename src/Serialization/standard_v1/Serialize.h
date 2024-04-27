#pragma once
#include "LogOperation.h"
#include "../../LogOperation.h"

namespace Serialization_standard_v1
{
  void Serialize(uint8_t * dst, const uint8_t * src, size_t length);

  template <class D, class S>
  void Serialize(D & dst, const S & src);

  template <class O>
  void SetLogOperationFooter(O & logOperation, size_t opSize);
};