#include <gtest/gtest.h>
#include <Core.h>
#include "helpers.h"

TEST(AttributeTest, AttributeInheritanceWorks)
{
  CoreTestWrapper wrapper;

  wrapper.types["type0"] = createTypeSpec([](OperationBuilder & builder)
  {
    builder.createNode("Map");
  }, wrapper.types);
  wrapper.types["type1"] = createTypeSpec([](OperationBuilder & builder)
  {
    builder.createContainerNode("type0", "type1wins");
  }, wrapper.types);
  wrapper.types["type2"] = createTypeSpec([](OperationBuilder & builder)
  {
    builder.createContainerNode("type1", "type2wins");
  }, wrapper.types);
  wrapper.types["type3"] = createTypeSpec([](OperationBuilder & builder)
  {
    builder.createContainerNode("type2", "type3wins");
  }, wrapper.types);
  wrapper.types["type4"] = createTypeSpec([](OperationBuilder & builder)
  {
    builder.createNode("type2");
  }, wrapper.types);
  wrapper.types["type5"] = createTypeSpec([](OperationBuilder & builder)
  {
    builder.createContainerNode("List", "type5wins");
  }, wrapper.types);
  wrapper.types["type6"] = createTypeSpec([](OperationBuilder & builder)
  {
    builder.createContainerNode("type5", "type6wins");
  }, wrapper.types);

  NodeId id1 = wrapper.builder.createNode("type3");
  NodeId id2 = wrapper.builder.createNode("type4");
  NodeId id3 = wrapper.builder.createNode("type6");

  wrapper.resolveTypes();

  const ContainerNode * containerNode;

  containerNode = reinterpret_cast<const ContainerNode *>(wrapper.core->getExistingNode(id1));
  ASSERT_NE(containerNode, nullptr);
  ASSERT_EQ(containerNode->getType().toString(), "type3");
  ASSERT_EQ(containerNode->getBaseType().toString(), "Map");
  ASSERT_EQ(containerNode->childType.getValueOrDefault().toString(), "type1wins");

  containerNode = reinterpret_cast<const ContainerNode *>(wrapper.core->getExistingNode(id2));
  ASSERT_NE(containerNode, nullptr);
  ASSERT_EQ(containerNode->getType().toString(), "type4");
  ASSERT_EQ(containerNode->getBaseType().toString(), "Map");
  ASSERT_EQ(containerNode->childType.getValueOrDefault().toString(), "type1wins");

  containerNode = reinterpret_cast<const ContainerNode *>(wrapper.core->getExistingNode(id3));
  ASSERT_NE(containerNode, nullptr);
  ASSERT_EQ(containerNode->getType().toString(), "type6");
  ASSERT_EQ(containerNode->getBaseType().toString(), "List");
  ASSERT_EQ(containerNode->childType.getValueOrDefault().toString(), "type5wins");
}