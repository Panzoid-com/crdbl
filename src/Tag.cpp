#include "Tag.h"

bool Tag::isNull() const
{
  return value[0] == 0 && value[1] == 0 && value[2] == 0 && value[3] == 0;
}

bool Tag::operator==(const Tag & rhs) const
{
  return value == rhs.value;
}

bool Tag::operator!=(const Tag & rhs) const
{
  return !(*this == rhs);
}

void Tag::reset()
{
  value[0] = value[1] = value[2] = value[3] = 0;
}

std::string Tag::toString() const
{
  std::stringstream out;

  for (auto i : this->value)
  {
    out << std::hex << std::setw(8) << std::setfill('0') << i;
  }

  return out.str();
}