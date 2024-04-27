#include "Json.h"

std::string DoubleToString(double value)
{
  std::string out;
  DoubleToString(value, out);
  return out;
}

void DoubleToString(double value, std::string & out)
{
  static char str[25];
  int len = sprintf(str, "%.17g", value);
  out.append(str, len);
}