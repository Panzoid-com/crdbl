#pragma once
#include "NodeType.h"

#include <string>
#include <unordered_map>

using PrimitiveNodeType = NodeType;

class PrimitiveNodeTypes
{
public:
  enum class PrimitiveType
  {
    Abstract,
    Null,

    Set,
    List,
    Map,
    OrderedFloat64Map,
    Reference,

    Int32Value,
    Int64Value,
    FloatValue,
    DoubleValue,
    Int8Value,
    BoolValue,
    StringValue
  };

  inline static const NodeType Abstract() { return NodeType(); }
  inline static const NodeType Null() { return NodeType("Null"); }

  inline static const NodeType Set() { return NodeType("Set"); }
  inline static const NodeType List() { return NodeType("List"); }
  inline static const NodeType Map() { return NodeType("Map"); }
  inline static const NodeType OrderedFloat64Map() { return NodeType("OrderedFloat64Map"); }
  inline static const NodeType Reference() { return NodeType("Reference"); }

  inline static const NodeType Int32Value() { return NodeType("Int32Value"); }
  inline static const NodeType Int64Value() { return NodeType("Int64Value"); }
  inline static const NodeType FloatValue() { return NodeType("FloatValue"); }
  inline static const NodeType DoubleValue() { return NodeType("DoubleValue"); }
  inline static const NodeType Int8Value() { return NodeType("Int8Value"); }
  inline static const NodeType BoolValue() { return NodeType("BoolValue"); }

  inline static const NodeType StringValue() { return NodeType("StringValue"); }

  static bool isPrimitiveNodeType(const NodeType & type) {
    return primitiveTypeMap().count(type.toString()) > 0;
  }

  static bool isNullNodeType(NodeType type) {
    return type == Null();
  }
  static bool isAbstractNodeType(NodeType type) {
    return type == Abstract();
  }
  static bool isContainerNodeType(NodeType type) {
    return type == Set() ||
      type == List() ||
      type == Map() ||
      type == OrderedFloat64Map() ||
      type == Reference();
  }
  static bool isValueNodeType(NodeType type) {
    return type == Int32Value() ||
      type == Int64Value() ||
      type == FloatValue() ||
      type == DoubleValue() ||
      type == Int8Value() ||
      type == BoolValue();
  }
  static bool isBlockValueNodeType(NodeType type) {
    return type == StringValue();
  }

  static PrimitiveType nodeTypeToPrimitiveType(const NodeType & type) {
    auto it = primitiveTypeMap().find(type.toString());
    if (it != primitiveTypeMap().end()) {
      return it->second;
    }
    return PrimitiveType::Abstract;
  }

  static std::unordered_map<std::string, NodeTypeData> getTypeRegistryInitializer();

private:
  static const std::unordered_map<std::string, PrimitiveType> & primitiveTypeMap() {
    static const std::unordered_map<std::string, PrimitiveType> map{
      {"", PrimitiveType::Abstract},
      {"Null", PrimitiveType::Null},
      {"Set", PrimitiveType::Set},
      {"List", PrimitiveType::List},
      {"Map", PrimitiveType::Map},
      {"OrderedFloat64Map", PrimitiveType::OrderedFloat64Map},
      {"Reference", PrimitiveType::Reference},
      {"Int32Value", PrimitiveType::Int32Value},
      {"Int64Value", PrimitiveType::Int64Value},
      {"FloatValue", PrimitiveType::FloatValue},
      {"DoubleValue", PrimitiveType::DoubleValue},
      {"Int8Value", PrimitiveType::Int8Value},
      {"BoolValue", PrimitiveType::BoolValue},
      {"StringValue", PrimitiveType::StringValue}
    };
    return map;
  }
};