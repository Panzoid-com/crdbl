#include "VectorTimestamp.h"

VectorTimestamp::VectorTimestamp()
{
  value = std::vector<uint32_t>();
  max = 0;
}

VectorTimestamp::VectorTimestamp(const VectorTimestamp & other)
  : VectorTimestamp(other.getVector()) {}

VectorTimestamp::VectorTimestamp(const std::vector<uint32_t> & vector)
  : VectorTimestamp(reinterpret_cast<const uint8_t *>(vector.data()), vector.size() * sizeof(uint32_t)) {}

VectorTimestamp::VectorTimestamp(const uint8_t * data, size_t length)
{
  const uint32_t * array = reinterpret_cast<const uint32_t *>(data);
  size_t arrayLength = length / sizeof(uint32_t);
  value.assign(array, array + arrayLength);

  uint32_t dataMax = 0;
  for (auto it = value.rbegin(); it != value.rend(); ++it)
  {
    if (*it > dataMax)
    {
      if (dataMax == 0 && it != value.rbegin())
      {
        //prune extraneous 0 values
        value.resize(value.size() - (it - value.rbegin()));
      }
      dataMax = *it;
    }
  }

  if (dataMax == 0)
  {
    value.clear();
  }

  max = dataMax;
}

bool VectorTimestamp::isEmpty() const
{
  std::unique_lock<std::mutex> lock(mutex);

  return value.size() == 0;
}

VectorTimestamp & VectorTimestamp::operator=(const VectorTimestamp & other)
{
  std::scoped_lock<std::mutex, std::mutex> lock(mutex, other.mutex);

  value = other.value;
  max = other.max;

  return *this;
}

bool VectorTimestamp::operator<(const VectorTimestamp & rhs) const
{
  std::scoped_lock<std::mutex, std::mutex> lock(mutex, rhs.mutex);

  if (value.size() < rhs.value.size())
  {
    return true;
  }

  for (uint32_t i = 0; i < rhs.value.size(); i++)
  {
    if (value[i] < rhs.value[i])
    {
      return true;
    }
  }

  return false;
}

bool VectorTimestamp::operator==(const VectorTimestamp & rhs) const
{
  std::scoped_lock<std::mutex, std::mutex> lock(mutex, rhs.mutex);

  if (value.size() != rhs.value.size())
  {
    return false;
  }

  for (uint32_t i = 0; i < rhs.value.size(); i++)
  {
    if (value[i] != rhs.value[i])
    {
      return false;
    }
  }

  return true;
}

bool VectorTimestamp::operator<(const Timestamp & rhs) const
{
  std::unique_lock<std::mutex> lock(mutex);

  uint32_t clock = 0;
  if (rhs.site < value.size())
  {
    clock = value[rhs.site];
  }

  return clock < rhs.clock;
}

bool VectorTimestamp::operator>=(const Timestamp & rhs) const
{
  std::unique_lock<std::mutex> lock(mutex);

  uint32_t clock = 0;
  if (rhs.site < value.size())
  {
    clock = value[rhs.site];
  }

  return clock >= rhs.clock;
}

void VectorTimestamp::update(const Timestamp & ts)
{
  std::unique_lock<std::mutex> lock(mutex);

  uint32_t clock = 0;
  if (ts.site < value.size())
  {
    clock = value[ts.site];
  }
  else if (ts.clock > 0)
  {
    value.resize(ts.site + 1, 0);
  }

  if (clock < ts.clock)
  {
    value[ts.site] = ts.clock;

    if (max < ts.clock)
    {
      max = ts.clock;
    }
  }
}

void VectorTimestamp::set(const Timestamp & ts)
{
  std::unique_lock<std::mutex> lock(mutex);

  uint32_t clock = 0;
  if (ts.site < value.size())
  {
    clock = value[ts.site];
  }
  else if (ts.clock > 0)
  {
    value.resize(ts.site + 1, 0);
  }

  value[ts.site] = ts.clock;

  if (clock == max && ts.clock < clock)
  {
    max = 0;
    for (auto e : value)
    {
      if (e > max)
      {
        max = e;
      }
    }
  }
  else if (max < ts.clock)
  {
    max = ts.clock;
  }
}

void VectorTimestamp::merge(const VectorTimestamp & other)
{
  std::scoped_lock<std::mutex, std::mutex> lock(mutex, other.mutex);

  if (other.value.size() > value.size())
  {
    value.resize(other.value.size());
  }

  for (int i = 0; i < other.value.size(); i++)
  {
    if (other.value[i] > value[i])
    {
      value[i] = other.value[i];
      if (value[i] > max)
      {
        max = value[i];
      }
    }
  }
}

void VectorTimestamp::reset()
{
  std::unique_lock<std::mutex> lock(mutex);

  value.clear();
  max = 0;
}

uint32_t VectorTimestamp::getClockAtSite(uint32_t site) const
{
  std::unique_lock<std::mutex> lock(mutex);

  if (site < value.size())
  {
    return value[site];
  }

  return 0;
}

Timestamp VectorTimestamp::getTimestampAtSite(uint32_t site) const
{
  std::unique_lock<std::mutex> lock(mutex);

  if (site < value.size())
  {
    return Timestamp(value[site], site);
  }

  return Timestamp::Null;
}

uint32_t VectorTimestamp::getMaxClock() const
{
  std::unique_lock<std::mutex> lock(mutex);

  return max;
}

std::vector<uint32_t> VectorTimestamp::getVector() const
{
  std::unique_lock<std::mutex> lock(mutex);

  return value;
}

std::string VectorTimestamp::toString() const
{
  std::unique_lock<std::mutex> lock(mutex);
  std::string outString;

  if (value.size() == 0)
  {
    return outString;
  }

  auto lastElement = std::prev(value.end());
  for (auto it = value.begin(); it != value.end(); ++it)
  {
    outString += std::to_string(*it);
    if (it != lastElement)
    {
      outString += ",";
    }
  }

  return outString;
}