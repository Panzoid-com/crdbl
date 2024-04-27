#pragma once
#include "../LogOperationSerialization.h" //for DeserializeDirection
#include "Format.h"
#include "LogOperation.h"
#include "LogOperationSerializer.h"
#include "LogOperationDeserializer.h"

namespace Serialization_standard_v1
{
  ILogOperationSerializer * CreateSerializer(const std::string & format);
  ILogOperationDeserializer * CreateDeserializer(const std::string & format, DeserializeDirection direction);
};