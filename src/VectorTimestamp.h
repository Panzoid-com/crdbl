#pragma once
#include <cstdint>
#include <string>
#include <vector>
#include <mutex>
#include "Timestamp.h"

class VectorTimestamp
{
public:
  VectorTimestamp();
  VectorTimestamp(const VectorTimestamp & other);
  VectorTimestamp(const std::vector<uint32_t> & vector);
  VectorTimestamp(const uint8_t * data, size_t length);

  bool isEmpty() const;
  VectorTimestamp & operator=(const VectorTimestamp & other);
  bool operator<(const VectorTimestamp & rhs) const;
  bool operator==(const VectorTimestamp & rhs) const;
  bool operator<(const Timestamp & rhs) const;
  bool operator>=(const Timestamp & rhs) const;
  void update(const Timestamp & ts);
  void set(const Timestamp & ts);
  void merge(const VectorTimestamp & other);
  void reset();
  uint32_t getClockAtSite(uint32_t site) const;
  Timestamp getTimestampAtSite(uint32_t site) const;
  uint32_t getMaxClock() const;
  std::vector<uint32_t> getVector() const;
  std::string toString() const;

private:
  mutable std::mutex mutex;
  std::vector<uint32_t> value;
  uint32_t max;
};