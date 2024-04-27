#include "PrimitiveNodeTypes.h"
#include "NodeType.h"

std::unordered_map<std::string, NodeTypeData> PrimitiveNodeTypes::getTypeRegistryInitializer() {
  return {
    {"Null", NodeTypeData{"Null", 1}},
    {"Set", NodeTypeData{"Set", 1}},
    {"List", NodeTypeData{"List", 1}},
    {"Map", NodeTypeData{"Map", 1}},
    {"OrderedFloat64Map", NodeTypeData{"OrderedFloat64Map", 1}},
    {"Reference", NodeTypeData{"Reference", 1}},
    {"Int32Value", NodeTypeData{"Int32Value", 1}},
    {"Int64Value", NodeTypeData{"Int64Value", 1}},
    {"FloatValue", NodeTypeData{"FloatValue", 1}},
    {"DoubleValue", NodeTypeData{"DoubleValue", 1}},
    {"Int8Value", NodeTypeData{"Int8Value", 1}},
    {"BoolValue", NodeTypeData{"BoolValue", 1}},
    {"StringValue", NodeTypeData{"StringValue", 1}}
  };
}