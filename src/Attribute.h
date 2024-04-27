#pragma once
#include "NodeType.h"
#include <unordered_map>
#include <string_view>
#include <string>

template <class T>
struct Attribute
{
  NodeType definedByType;
  T value;

  bool hasValue() const;
  T getValueOrDefault() const;
};

using AttributeId = uint16_t;
using AttributeData = std::basic_string<char>;
using AttributeMap = std::unordered_map<AttributeId, std::pair<NodeType, AttributeData>>;

void setAttributeValue(AttributeMap & attributes, const AttributeId & id,
  const NodeType & definedByType, const char * data, size_t length);

NodeType getAttributeDefinedByType(const AttributeMap & attributes, const AttributeId & id);

template <class T>
T getAttributeValueOrDefault(const AttributeMap & attributes, const AttributeId & id);

template <class T>
T getAttributeValue(const AttributeMap & attributes, const AttributeId & id,
  bool & success);

