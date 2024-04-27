#include "Node.h"

bool Node::isAbstractType() const
{
  if (type.size() == 0)
  {
    return true;
  }

  return false;
}

void Node::addType(NodeType newType)
{
  if (newType == nullptr)
  {
    return;
  }

  type.push_back(newType);
}

NodeType Node::getType() const
{
  if (type.size() > 0)
  {
    return type.front();
  }

  return PrimitiveNodeTypes::Abstract();
}

NodeType Node::getBaseType() const
{
  if (type.size() > 0)
  {
    return type.back();
  }

  return PrimitiveNodeTypes::Abstract();
}

bool Node::isDerivedFromType(NodeType baseType) const
{
  if (baseType == PrimitiveNodeTypes::Abstract())
  {
    return true;
  }

  if (std::find(type.begin(), type.end(), baseType) != type.end())
  {
    return true;
  }

  return false;
}

void Node::serialize(IObjectSerializer & serializer) const
{
  serializer.startObject();
  if (effect.isVisible())
  {
    serializer.addPair("ready", true);
  }

  serializer.addKey("type");
  serializer.startArray();
  for (auto it = type.begin(); it != type.end(); ++it)
  {
    serializer.addValue((*it).toString());
  }
  serializer.endArray();

  serializer.endObject();
}