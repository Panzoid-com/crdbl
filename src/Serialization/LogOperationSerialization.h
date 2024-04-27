#pragma once
#include "ILogOperationSerializer.h"
#include "ILogOperationDeserializer.h"
#include <string>

enum DeserializeDirection { Forward, Reverse };

class LogOperationSerialization
{
public:
  static const char * DefaultFormat();
  static const char * DefaultTypeFormat();
  static ILogOperationSerializer * CreateSerializer(const std::string & format);
  static ILogOperationDeserializer * CreateDeserializer(const std::string & format, DeserializeDirection direction);
};