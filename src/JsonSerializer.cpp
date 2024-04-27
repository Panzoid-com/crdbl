#include "JsonSerializer.h"
#include "Json.h"

void JsonSerializer::startObject()
{
  objectStartStack.push(output.tellp());
  output << "{";
}

void JsonSerializer::endObject()
{
  if (objectStartStack.top() != output.tellp())
  {
    output.seekp(-1, output.cur);
  }
  objectStartStack.pop();
  output << "}";
}

void JsonSerializer::resumeObject()
{
  output.seekp(-1, output.cur);
  output << ",";
}

void JsonSerializer::startArray()
{
  objectStartStack.push(output.tellp());
  output << "[";
}

void JsonSerializer::endArray()
{
  if (objectStartStack.top() != output.tellp())
  {
    output.seekp(-1, output.cur);
  }
  objectStartStack.pop();
  output << "]";
}

void JsonSerializer::resumeArray()
{
  output.seekp(-1, output.cur);
  output << ",";
}

void JsonSerializer::addPair(const std::string & key, bool value)
{
  addKey(key);
  output << (value ? "true" : "false") << ",";
}

void JsonSerializer::addPair(const std::string & key, const std::string & value)
{
  addKey(key);
  output << "\"" << value << "\",";
}

void JsonSerializer::addPair(const std::string & key, const char * value)
{
  addKey(key);
  output << "\"" << value << "\",";
}

void JsonSerializer::addPair(const std::string & key, const Timestamp & value)
{
  addKey(key);
  output << "\"" << value.toString() << "\",";
}

void JsonSerializer::addPair(const std::string & key, const NodeId & value)
{
  addKey(key);
  output << "\"" << value.toString() << "\",";
}

void JsonSerializer::addPair(const std::string & key, size_t value)
{
  addKey(key);
  output << value << ",";
}

void JsonSerializer::addPair(const std::string & key, uint32_t value)
{
  addKey(key);
  output << value << ",";
}

void JsonSerializer::addPair(const std::string & key, double value)
{
  addKey(key);
  output << DoubleToString(value) << ",";
}

void JsonSerializer::addPair(const std::string & key, float value)
{
  addKey(key);
  output << DoubleToString(value) << ",";
}

void JsonSerializer::addPair(const std::string & key, int32_t value)
{
  addKey(key);
  output << value << ",";
}

void JsonSerializer::addPair(const std::string & key, int64_t value)
{
  addKey(key);
  output << value << ",";
}

void JsonSerializer::addPair(const std::string & key, int8_t value)
{
  addKey(key);
  output << value;
}

void JsonSerializer::addKey(const std::string & key)
{
  output << "\"" << key << "\":";
}

void JsonSerializer::addValue(const std::string & value)
{
  output << "\"" << value << "\",";
}

void JsonSerializer::addNullValue()
{
  output << "null" << ",";
}

std::string JsonSerializer::result()
{
  return output.str();
}