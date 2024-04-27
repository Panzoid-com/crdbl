#pragma once
#include <cstdint>
#include <string>
#include "Timestamp.h"

struct BlockValueCacheItem
{
  BlockValueCacheItem() {}
  BlockValueCacheItem(const Timestamp & firstWrite)
    : firstWrite(firstWrite) {}

  Timestamp firstWrite;
  std::basic_string<char> value;
  int32_t indexOffset = 0;
};