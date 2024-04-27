#pragma once
#include <cstdint>
#include <array>

#include <sstream>
#include <iomanip>

struct Tag
{
  std::array<uint32_t, 4> value;

  bool operator==(const Tag & rhs) const;
  bool operator!=(const Tag & rhs) const;
  static constexpr Tag Default()
  {
    return { 0 };
  }
  static constexpr size_t SizeBytes()
  {
    return 4 * sizeof(uint32_t);
  }
  bool isNull() const;
  std::string toString() const;
  void reset();
};

namespace std
{
  template <>
  struct hash<Tag>
  {
    std::size_t operator()(const Tag & tag) const
    {
      size_t seed = 0;
      for (auto i : tag.value)
      {
        seed ^= hash<uint8_t>{}(i) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
      }
      return seed;
    }
  };
}