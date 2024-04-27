#include <gtest/gtest.h>
#include <algorithm>
#include <concepts> //might need g++-10 or another more recent-ish stdlib
#include <Core.h>
#include "helpers.h"

TEST(CoreTest, InheritanceWorks)
{
  CoreTestWrapper wrapper;

  wrapper.types["type0"] = createTypeSpec([](OperationBuilder & builder)
  {
    NodeId rootId = builder.createNode(PrimitiveNodeTypes::Map());

    builder.addChild(rootId, builder.createNode(PrimitiveNodeTypes::DoubleValue()), "key1");
    builder.addChild(rootId, builder.createNode(PrimitiveNodeTypes::DoubleValue()), "key2");

    NodeId mapId = builder.createNode(PrimitiveNodeTypes::Map());
    builder.addChild(rootId, mapId, "key3");

    builder.addChild(mapId, builder.createNode(PrimitiveNodeTypes::DoubleValue()), "key1");
    builder.addChild(mapId, builder.createNode(PrimitiveNodeTypes::DoubleValue()), "key2");
    builder.addChild(mapId, builder.createNode(PrimitiveNodeTypes::DoubleValue()), "key3");
  }, wrapper.types);
  wrapper.types["type1"] = createTypeSpec([](OperationBuilder & builder)
  {
    NodeId rootId = builder.createNode("type0");
    builder.addChild(rootId, builder.createNode(PrimitiveNodeTypes::DoubleValue()), "key4");
  }, wrapper.types);
  wrapper.types["type2"] = createTypeSpec([](OperationBuilder & builder)
  {
    NodeId rootId = builder.createNode("type1");
    builder.addChild(rootId, builder.createNode(PrimitiveNodeTypes::DoubleValue()), "key5");
  }, wrapper.types);

  NodeId id = wrapper.builder.createNode("type2");

  const Node * node;
  node = reinterpret_cast<const Node *>(wrapper.core->getExistingNode(id));
  ASSERT_NE(node, nullptr);
  EXPECT_EQ(node->effect.isVisible(), false);

  wrapper.resolveTypes();

  node = reinterpret_cast<const Node *>(wrapper.core->getExistingNode(id));
  ASSERT_NE(node, nullptr);
  EXPECT_EQ(node->effect.isVisible(), true);
  EXPECT_EQ(node->getType().toString(), "type2");
  EXPECT_EQ(node->getBaseType().toString(), "Map");
  EXPECT_EQ(node->type, std::vector<NodeType>({
    NodeType("type2"), NodeType("type1"), NodeType("type0"), NodeType("Map") }));
}

TEST(CoreTest, BuiltInNodesExist)
{
  CoreTestWrapper wrapper;

  auto node = wrapper.core->getExistingNode(NodeId::Root);
  ASSERT_NE(node, nullptr);
  EXPECT_EQ(node->getType(), PrimitiveNodeTypes::Abstract());
  EXPECT_EQ(node->effect.isVisible(), true);

  node = wrapper.core->getExistingNode(NodeId::Null);
  ASSERT_NE(node, nullptr);
  EXPECT_EQ(node->getType(), PrimitiveNodeTypes::Null());
  EXPECT_EQ(node->effect.isVisible(), true);

  node = wrapper.core->getExistingNode(NodeId::Pending);
  ASSERT_NE(node, nullptr);
  EXPECT_EQ(node->isAbstractType(), true);
  EXPECT_EQ(node->effect.isVisible(), true);
}

TEST(CoreTest, FirstNodeGeneratesEvent)
{
  int eventCount = 0;
  auto handleEvent = [&](const CoreTestWrapper & wrapper, const Event & event)
  {
    auto addedEvent = dynamic_cast<const NodeAddedEvent *>(&event);
    if (addedEvent != nullptr)
    {
      EXPECT_EQ(addedEvent->parentId, NodeId::Root);
      EXPECT_EQ(addedEvent->edgeId, NodeId::Null);
      EXPECT_EQ(addedEvent->childId, NodeId::SiteRoot);
      EXPECT_EQ(addedEvent->speculative, false);

      eventCount++;
    }
  };

  CoreTestWrapper wrapper(handleEvent);
  NodeId id = wrapper.builder.createNode(PrimitiveNodeTypes::Map());
  EXPECT_EQ(eventCount, 1);

  eventCount = 0;
  CoreTestWrapper wrapper2(handleEvent);
  NodeId id2 = wrapper2.builder.createNode("type0");
  EXPECT_EQ(eventCount, 0);

  wrapper2.types["type0"] = createTypeSpec([](OperationBuilder & builder)
  {
    NodeId rootId = builder.createNode(PrimitiveNodeTypes::Map());
  }, wrapper2.types);
  wrapper2.resolveTypes();

  EXPECT_EQ(eventCount, 1);
}

TEST(CoreTest, AllOperationsUpdateVectorClock)
{
  static_assert(sizeof(OperationType) == sizeof(uint8_t),
    "assuming operation type is a uint8_t");

  CoreTestWrapper wrapper;

  //try creating operations with all possible OperationType values
  for (int type = 0; type <= 0xFF; type++)
  {
    //allocate a buffer large enough to at least hold the minimum size of any op
    const size_t bufferSize = 1024;
    auto opBuffer = new uint8_t[bufferSize];

    Timestamp prevTs = { wrapper.core->clock.getClockAtSite(1), 1 };

    auto tsOp = reinterpret_cast<LogOperation *>(opBuffer);
    std::memset(opBuffer, 0, bufferSize);
    tsOp->ts = prevTs + 1;
    tsOp->op.type = static_cast<OperationType>(type);

    if (tsOp->op.getSize() == 0)
    {
      //assert that actual operations aren't getting here
      ASSERT_NE(type, static_cast<uint8_t>(OperationType::NoOpOperation));
      continue;
    }

    //verify test assumption that the op fits in the buffer
    ASSERT_LT(tsOp->op.getSize(), bufferSize);

    RefCounted<const LogOperation> rc(tsOp);
    wrapper.core->applyOperation(rc);

    if (!tsOp->op.isGroup()
      && tsOp->op.type != OperationType::ValuePreviewOperation)
    {
      EXPECT_EQ(tsOp->ts.clock, wrapper.core->clock.getClockAtSite(1));
    }
    else
    {
      //empty groups don't actually increment the clock
      EXPECT_EQ(prevTs.clock, wrapper.core->clock.getClockAtSite(1));
    }
  }
}

TEST(CoreTest, GroupOperationsUpdateVectorClock)
{
  CoreTestWrapper wrapper;

  EXPECT_EQ(wrapper.core->clock.getClockAtSite(1), 0);

  wrapper.group([&](OperationBuilder & builder)
  {
    builder.createNode(PrimitiveNodeTypes::Map());
    builder.createNode(PrimitiveNodeTypes::Map());
    builder.createNode(PrimitiveNodeTypes::Map());
  });

  EXPECT_EQ(wrapper.core->clock.getClockAtSite(1), 3);
}