#include "LogOperationSerialization.h"

#include "standard_v1/LogOperationSerialization.h"

const char * LogOperationSerialization::DefaultFormat()
{
  return "standard_v1_full";
}

const char * LogOperationSerialization::DefaultTypeFormat()
{
  return "standard_v1_type";
}

ILogOperationSerializer * LogOperationSerialization::CreateSerializer(const std::string & format)
{
  return Serialization_standard_v1::CreateSerializer(format);
}

ILogOperationDeserializer * LogOperationSerialization::CreateDeserializer(const std::string & format, DeserializeDirection direction)
{
  return Serialization_standard_v1::CreateDeserializer(format, direction);
}