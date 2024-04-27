#include <gtest/gtest.h>
#include <Core.h>
#include "helpers.h"

TEST(NodeTest, CreateNodeWorks)
{
  CoreTestWrapper wrapper;

  NodeId id1 = wrapper.builder.createNode(PrimitiveNodeTypes::Abstract());
  const Node * node0 = wrapper.core->getExistingNode(id1);

  ASSERT_NE(node0, (Node *)nullptr);
  ASSERT_EQ(node0->getBaseType(), PrimitiveNodeTypes::Abstract());

  NodeId id2 = wrapper.builder.createNode(PrimitiveNodeTypes::List());
  const Node * node1 = wrapper.core->getExistingNode(id2);

  ASSERT_NE(node1, (Node *)nullptr);
  ASSERT_EQ(node1->getBaseType(), PrimitiveNodeTypes::List());
}

TEST(NodeTest, DoubleCreateNodeDoesNothing)
{
  int eventCount = 0;
  CoreTestWrapper wrapper([&](const CoreTestWrapper & wrapper, const Event & event)
  {
    eventCount++;
  });

  NodeId id1 = wrapper.builder.createNode(PrimitiveNodeTypes::Abstract());
  const Node * node0 = wrapper.core->getExistingNode(id1);
  EXPECT_NE(node0, (Node *)nullptr);
  EXPECT_EQ(eventCount, 1);

  //reapply the same operation
  applyOperation(wrapper, wrapper.log, id1.ts);

  const Node * node1 = wrapper.core->getExistingNode(id1);
  EXPECT_EQ(node0, node1);
  EXPECT_EQ(eventCount, 1);
}