#include <gtest/gtest.h>
#include <Core.h>
#include "helpers.h"

TEST(ReferenceNodeTest, InsertWorks)
{
  CoreTestWrapper wrapper;

  auto referenceNodeId = wrapper.builder.createNode(PrimitiveNodeTypes::Reference());

  auto childId0 = wrapper.builder.createNode(PrimitiveNodeTypes::Abstract());
  auto edgeId0 = wrapper.builder.addChild(referenceNodeId, childId0);

  auto result = wrapper.getReferenceNodeChildren(referenceNodeId);
  ASSERT_EQ(result.size(), 1);
  ASSERT_EQ(result[0].first, edgeId0);
  ASSERT_EQ(result[0].second, childId0);
}

TEST(ReferenceNodeTest, RemoveWorks)
{
  CoreTestWrapper wrapper;

  auto referenceNodeId = wrapper.builder.createNode(PrimitiveNodeTypes::Reference());

  auto childId0 = wrapper.builder.createNode(PrimitiveNodeTypes::Abstract());
  auto edgeId0 = wrapper.builder.addChild(referenceNodeId, childId0);

  wrapper.builder.removeChild(referenceNodeId, edgeId0);

  auto result = wrapper.getReferenceNodeChildren(referenceNodeId);
  ASSERT_EQ(result.size(), 0);
}

TEST(ReferenceNodeTest, InsertOverwritesPrevious)
{
  CoreTestWrapper wrapper;

  auto referenceNodeId = wrapper.builder.createNode(PrimitiveNodeTypes::Reference());

  auto childId0 = wrapper.builder.createNode(PrimitiveNodeTypes::Abstract());
  auto edgeId0 = wrapper.builder.addChild(referenceNodeId, childId0);

  auto childId1 = wrapper.builder.createNode(PrimitiveNodeTypes::Abstract());
  auto edgeId1 = wrapper.builder.addChild(referenceNodeId, childId1);

  auto result = wrapper.getReferenceNodeChildren(referenceNodeId);
  ASSERT_EQ(result.size(), 1);
  ASSERT_EQ(result[0].first, edgeId1);
  ASSERT_EQ(result[0].second, childId1);
}

TEST(ReferenceNodeTest, ChildTypeAttributeEnforcesChildType)
{
  CoreTestWrapper wrapper;

  NodeType childType = PrimitiveNodeTypes::Int32Value();
  auto referenceNodeId = wrapper.builder.createContainerNode(PrimitiveNodeTypes::Reference(),
    childType);

  wrapper.builder.addChild(referenceNodeId,
    wrapper.builder.createNode(PrimitiveNodeTypes::Abstract()), "key");

  auto result0 = wrapper.getReferenceNodeChildren(referenceNodeId);
  ASSERT_EQ(result0.size(), 0);

  auto edgeId1 = wrapper.builder.addChild(referenceNodeId,
    wrapper.builder.createNode(childType));

  auto result1 = wrapper.getReferenceNodeChildren(referenceNodeId);
  ASSERT_EQ(result1.size(), 1);
  ASSERT_EQ(result1[0].first, edgeId1);
}