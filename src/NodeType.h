#pragma once
#include <string>
#include <unordered_map>

struct NodeTypeData
{
  std::string type;
  size_t refCount = 0;
};

class NodeType
{
public:
  NodeType() = default;
  explicit NodeType(const std::string & type);
  NodeType(NodeTypeData * data);
  NodeType(const NodeType & other);
  NodeType(NodeType && other);

  ~NodeType();

  bool operator==(const NodeType & rhs) const;
  bool operator!=(const NodeType & rhs) const;
  NodeType & operator=(const NodeType & rhs);

  std::string toString() const;

private:
  static std::unordered_map<std::string, NodeTypeData> nodeTypes;

  static NodeTypeData & getOrCreateNodeType(const std::string & type);

  NodeTypeData * data = nullptr;

  friend struct std::hash<NodeType>;
};

namespace std
{
  template <>
  struct hash<NodeType>
  {
    std::size_t operator()(const NodeType & nodeType) const
    {
      //hash data like a pointer
      size_t seed = 0;
      seed ^= hash<NodeTypeData *>()(nodeType.data) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
      return seed;
    }
  };
}