#pragma once
#include <cstdint>
#include <string>
#include <functional>

class Timestamp
{
public:
  uint32_t clock = 0;
  uint32_t site = 0;

  Timestamp() : clock(0), site(0) {}
  Timestamp(uint32_t clock, uint32_t site) : clock(clock), site(site) {}

  bool operator<(const Timestamp & rhs) const;
  bool operator>(const Timestamp & rhs) const;
  bool operator==(const Timestamp & rhs) const = default;
  bool operator!=(const Timestamp & rhs) const = default;
  Timestamp & operator++();
  Timestamp & operator+=(const Timestamp & rhs);
  friend Timestamp operator+(const Timestamp & lhs, unsigned int rhs);
  friend Timestamp operator+(const Timestamp & lhs, const Timestamp & rhs);
  void update(const Timestamp & ts);
  bool isTypeRoot() const;
  bool isNull() const;
  bool isFinal() const;
  std::string toString() const;

  static const Timestamp Null;
  static const Timestamp Root;
  static const Timestamp Final;
};

namespace std
{
  template <>
  struct hash<Timestamp>
  {
    std::size_t operator()(const Timestamp & ts) const
    {
      size_t seed = 0;
      seed ^= hash<uint32_t>()(ts.clock) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
      seed ^= hash<uint32_t>()(ts.site) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
      return seed;
    }
  };
}