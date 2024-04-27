#include "Attribute.h"
#include "PrimitiveNodeTypes.h"
#include "EdgeId.h"

template <class T>
bool Attribute<T>::hasValue() const
{
  return definedByType != PrimitiveNodeTypes::Abstract();
}

template <class T>
T Attribute<T>::getValueOrDefault() const
{
  if (hasValue())
  {
    return value;
  }

  return T();
}

template struct Attribute<NodeType>;

void setAttributeValue(AttributeMap & attributes, const AttributeId & id,
  const NodeType & definedByType, const char * data, size_t length)
{
  AttributeData attrData(data, length);
  attributes[id] = std::make_pair(definedByType, attrData);
}

NodeType getAttributeDefinedByType(const AttributeMap & attributes, const AttributeId & id)
{
  auto it = attributes.find(id);
  if (it == attributes.end())
  {
    return PrimitiveNodeTypes::Abstract();
  }

  return it->second.first;
}

template <class T>
T getAttributeValueOrDefault(const AttributeMap & attributes, const AttributeId & id)
{
  auto it = attributes.find(id);
  if (it == attributes.end())
  {
    return T();
  }

  if (it->second.second.length() != sizeof(T))
  {
    return T();
  }

  return *reinterpret_cast<const T*>(it->second.second.data());
}

template <class T>
T getAttributeValue(const AttributeMap & attributes, const AttributeId & id,
  bool & success)
{
  auto it = attributes.find(id);
  if (it == attributes.end())
  {
    success = false;
    return T();
  }

  if (it->second.second.length() != sizeof(T))
  {
    success = false;
    return T();
  }

  success = true;
  return *reinterpret_cast<const T*>(it->second.second.data());
}

template <>
std::string getAttributeValueOrDefault(const AttributeMap & attributes, const AttributeId & id)
{
  auto it = attributes.find(id);
  if (it == attributes.end())
  {
    return std::string();
  }

  return std::string(it->second.second);
}

template <>
std::string getAttributeValue(const AttributeMap & attributes, const AttributeId & id,
  bool & success)
{
  auto it = attributes.find(id);
  if (it == attributes.end())
  {
    success = false;
    return std::string();
  }

  success = true;
  return std::string(it->second.second);
}

template EdgeId getAttributeValueOrDefault(const AttributeMap & attributes, const AttributeId & id);
template double getAttributeValue(const AttributeMap & attributes, const AttributeId & id, bool & success);