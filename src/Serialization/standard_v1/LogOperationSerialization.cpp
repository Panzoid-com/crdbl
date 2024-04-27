#include "LogOperationSerialization.h"

namespace Serialization_standard_v1
{
  ILogOperationSerializer * CreateSerializer(const std::string & format)
  {
    ILogOperationSerializer * serializer = nullptr;

    if (format.starts_with("standard_v1"))
    {
      size_t padding = (format.ends_with("_padded")) ? 1 : 0;

      if (format.starts_with("standard_v1_full"))
      {
        serializer = new LogOperationSerializer<Subformat::Full>(padding);
      }
      else if (format.starts_with("standard_v1_untagged"))
      {
        serializer = new LogOperationSerializer<Subformat::Untagged>(padding);
      }
      else if (format.starts_with("standard_v1_forward"))
      {
        serializer = new LogOperationSerializer<Subformat::Forward>(padding);
      }
      else if (format == "standard_v1_type")
      {
        serializer = new LogOperationSerializer<Subformat::Type>(0);
      }
    }

    return serializer;
  }

  ILogOperationDeserializer * CreateDeserializer(const std::string & format, DeserializeDirection direction)
  {
    ILogOperationDeserializer * deserializer = nullptr;

    if (format.starts_with("standard_v1"))
    {
      if (format == "standard_v1_full")
      {
        if (direction == DeserializeDirection::Forward)
        {
          deserializer = new LogOperationDeserializer<
            Subformat::Full, DeserializeDirection::Forward>();
        }
        else
        {
          deserializer = new LogOperationDeserializer<
            Subformat::Full, DeserializeDirection::Reverse>();
        }
      }
      else if (format == "standard_v1_untagged")
      {
        if (direction == DeserializeDirection::Forward)
        {
          deserializer = new LogOperationDeserializer<
            Subformat::Untagged, DeserializeDirection::Forward>();
        }
        else
        {
          deserializer = new LogOperationDeserializer<
            Subformat::Untagged, DeserializeDirection::Reverse>();
        }
      }
      else if (format == "standard_v1_forward")
      {
        if (direction == DeserializeDirection::Forward)
        {
          deserializer = new LogOperationDeserializer<
            Subformat::Forward, DeserializeDirection::Forward>();
        }
      }
      else if (format == "standard_v1_type")
      {
        if (direction == DeserializeDirection::Forward)
        {
          deserializer = new LogOperationDeserializer<
            Subformat::Type, DeserializeDirection::Forward>();
        }
      }
    }

    return deserializer;
  }
};