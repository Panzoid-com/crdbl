#include "Timestamp.h"

const uint32_t MAX_TS_CLOCK = 0xFFFFFFFF;

bool Timestamp::operator<(const Timestamp & rhs) const
{
  return clock < rhs.clock || (clock == rhs.clock && site < rhs.site);
}

bool Timestamp::operator>(const Timestamp & rhs) const
{
  return clock > rhs.clock || (clock == rhs.clock && site > rhs.site);
}

Timestamp & Timestamp::operator++()
{
  clock++;

  return *this;
}

Timestamp & Timestamp::operator+=(const Timestamp & rhs)
{
  site += rhs.site;
  clock += rhs.clock;

  return *this;
}

Timestamp operator+(const Timestamp & lhs, unsigned int rhs)
{
  Timestamp newTs;
  newTs.site = lhs.site;
  newTs.clock = lhs.clock + rhs;
  return newTs;
}

Timestamp operator+(const Timestamp & lhs, const Timestamp & rhs)
{
  Timestamp newTs;
  newTs.site = lhs.site + rhs.site;
  newTs.clock = lhs.clock + rhs.clock;
  return newTs;
}

void Timestamp::update(const Timestamp & ts)
{
  if (ts.clock > clock)
  {
    clock = ts.clock;
  }
}

bool Timestamp::isTypeRoot() const
{
  return *this == Root;
}

bool Timestamp::isNull() const
{
  return *this == Null;
}

bool Timestamp::isFinal() const
{
  return clock == MAX_TS_CLOCK;
}

std::string Timestamp::toString() const
{
  std::string outString;
  outString += std::to_string(clock);
  outString += ",";
  outString += std::to_string(site);
  return outString;
}

const Timestamp Timestamp::Null = { 0, 0 };
const Timestamp Timestamp::Root = { 1, 0 };
const Timestamp Timestamp::Final = { MAX_TS_CLOCK, 0 };