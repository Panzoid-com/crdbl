#include "NodeType.h"
#include "PrimitiveNodeTypes.h"

std::unordered_map<std::string, NodeTypeData> NodeType::nodeTypes =
  PrimitiveNodeTypes::getTypeRegistryInitializer();

NodeType::NodeType(const std::string & type)
{
  if (!type.empty())
  {
    data = &getOrCreateNodeType(type);
    data->refCount++;
  }
}

NodeType::NodeType(NodeTypeData * data)
  : data(data)
{
  if (data != nullptr)
  {
    data->refCount++;
  }
}

NodeType::NodeType(const NodeType & other)
  : data(other.data)
{
  if (data != nullptr)
  {
    data->refCount++;
  }
}

NodeType::NodeType(NodeType && other)
  : data(other.data)
{
  other.data = nullptr;
}

bool NodeType::operator==(const NodeType & rhs) const
{
  return data == rhs.data;
}

bool NodeType::operator!=(const NodeType & rhs) const
{
  return data != rhs.data;
}

NodeType & NodeType::operator=(const NodeType & rhs)
{
  if (data != nullptr)
  {
    data->refCount--;

    if (data->refCount == 0)
    {
      nodeTypes.erase(data->type);
    }
  }

  data = rhs.data;

  if (data != nullptr)
  {
    data->refCount++;
  }

  return *this;
}

NodeType::~NodeType()
{
  if (data != nullptr)
  {
    data->refCount--;

    if (data->refCount == 0)
    {
      nodeTypes.erase(data->type);
    }
  }
}

std::string NodeType::toString() const
{
  if (data == nullptr)
  {
    return std::string();
  }

  return data->type;
}

NodeTypeData & NodeType::getOrCreateNodeType(const std::string & type)
{
  auto [iter, inserted] = nodeTypes.try_emplace(type, NodeTypeData{type});
  return iter->second;
}