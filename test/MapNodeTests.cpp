#include <gtest/gtest.h>
#include <algorithm>
#include <Core.h>
#include "helpers.h"

TEST(MapNodeTest, InsertWorks)
{
  CoreTestWrapper wrapper;

  auto mapNodeId = wrapper.builder.createNode(PrimitiveNodeTypes::Map());

  auto childId0 = wrapper.builder.createNode(PrimitiveNodeTypes::Abstract());
  auto edgeId0 = wrapper.builder.addChild(mapNodeId, childId0, "key");

  auto result = wrapper.getMapNodeChildren(mapNodeId);
  ASSERT_EQ(result.size(), 1);
  ASSERT_EQ(result["key"].first, edgeId0);
  ASSERT_EQ(result["key"].second, childId0);
}

TEST(MapNodeTest, RemoveWorks)
{
  CoreTestWrapper wrapper;

  auto mapNodeId = wrapper.builder.createNode(PrimitiveNodeTypes::Map());

  auto childId0 = wrapper.builder.createNode(PrimitiveNodeTypes::Abstract());
  auto edgeId0 = wrapper.builder.addChild(mapNodeId, childId0, "key");

  wrapper.builder.removeChild(mapNodeId, edgeId0);

  auto result = wrapper.getMapNodeChildren(mapNodeId);
  ASSERT_EQ(result.size(), 0);
}

TEST(MapNodeTest, ReplaceKeyWorks)
{
  NodeId oldChildId;
  NodeId newChildId;
  bool nodeRemoved = false;
  bool nodeAdded = false;

  CoreTestWrapper wrapper([&](const CoreTestWrapper & wrapper, const Event & event)
  {
    auto addedEvent = dynamic_cast<const NodeAddedEventMapped *>(&event);
    if (addedEvent != nullptr)
    {
      if (addedEvent->childId == NodeId::SiteRoot)
      {
        return;
      }

      if (nodeRemoved == false)
      {
        ASSERT_EQ(oldChildId, addedEvent->childId);
        ASSERT_FALSE(nodeAdded);
      }
      else
      {
        ASSERT_EQ(newChildId, addedEvent->childId);
        ASSERT_FALSE(nodeAdded);
        nodeAdded = true;
      }

      return;
    }

    auto removedEvent = dynamic_cast<const NodeRemovedEventMapped *>(&event);
    if (removedEvent != nullptr)
    {
      ASSERT_EQ(oldChildId, removedEvent->childId);
      ASSERT_FALSE(nodeRemoved);
      nodeRemoved = true;

      return;
    }
  });

  auto mapNodeId = wrapper.builder.createNode(PrimitiveNodeTypes::Map());

  oldChildId = wrapper.builder.createNode(PrimitiveNodeTypes::Abstract());
  wrapper.builder.addChild(mapNodeId, oldChildId, "key");

  newChildId = wrapper.builder.createNode(PrimitiveNodeTypes::Abstract());
  auto edgeId = wrapper.builder.addChild(mapNodeId, newChildId, "key");

  auto result = wrapper.getMapNodeChildren(mapNodeId);
  ASSERT_EQ(result.size(), 1);
  ASSERT_EQ(result["key"].first, edgeId);
  ASSERT_EQ(result["key"].second, newChildId);
  ASSERT_TRUE(nodeAdded);
  ASSERT_TRUE(nodeRemoved);
}

TEST(MapNodeTest, ChildTypeAttributeEnforcesChildType)
{
  CoreTestWrapper wrapper;

  NodeType childType = PrimitiveNodeTypes::Int32Value();
  auto nodeId = wrapper.builder.createContainerNode(PrimitiveNodeTypes::Map(),
    childType);

  wrapper.builder.addChild(nodeId,
    wrapper.builder.createNode(PrimitiveNodeTypes::Abstract()), "key");

  auto result0 = wrapper.getMapNodeChildren(nodeId);
  ASSERT_EQ(result0.size(), 0);

  auto edgeId1 = wrapper.builder.addChild(nodeId,
    wrapper.builder.createNode(childType), "key");

  auto result1 = wrapper.getMapNodeChildren(nodeId);
  ASSERT_EQ(result1.size(), 1);
  ASSERT_EQ(result1["key"].first, edgeId1);
}

TEST(MapNodeTest, InheritanceWorks)
{
  CoreTestWrapper wrapper;

  std::unordered_map<std::string, std::stack<int32_t>> expectedState;
  std::unordered_map<std::string, std::stack<int32_t>> expectedState2;

  wrapper.types["type0"] = createTypeSpec([](CoreTestWrapper & wrapper)
  {
    wrapper.builder.createNode(PrimitiveNodeTypes::Map());
  }, wrapper.types);
  wrapper.types["type1"] = createTypeSpecFromRoot("type0",
  [&](const NodeId & rootNodeId, CoreTestWrapper & wrapper)
  {
    NodeId nodeId;
    EdgeId edgeId = EdgeId::Null;

    for (int i = 0; i < 10; i++)
    {
      nodeId = wrapper.builder.createNode(PrimitiveNodeTypes::Int32Value());
      int32_t value = 100 + i;
      wrapper.builder.setValue(nodeId, value);
      std::string key = "key" + std::to_string(i);
      edgeId = wrapper.builder.addChild(rootNodeId, nodeId, key);
      expectedState[key].push(value);
    }
  }, wrapper.types);
  wrapper.types["type2"] = createTypeSpecFromRoot("type1",
  [&](const NodeId & rootNodeId, CoreTestWrapper & wrapper)
  {
    NodeId nodeId;
    EdgeId edgeId;

    for (int i = 0; i < 10; i++)
    {
      nodeId = wrapper.builder.createNode(PrimitiveNodeTypes::Int32Value());
      int32_t value = 200 + i;
      wrapper.builder.setValue(nodeId, value);
      std::string key = "key" + std::to_string(i + 5);
      edgeId = wrapper.builder.addChild(rootNodeId, nodeId, key);
      expectedState[key].push(value);
    }
  }, wrapper.types);
  wrapper.types["type3"] = createTypeSpecFromRoot("type2",
  [&](const NodeId & rootNodeId, CoreTestWrapper & wrapper)
  {
    NodeId nodeId;
    EdgeId edgeId;

    for (int i = 0; i < 10; i++)
    {
      nodeId = wrapper.builder.createNode(PrimitiveNodeTypes::Int32Value());
      int32_t value = 300 + i;
      wrapper.builder.setValue(nodeId, value);
      std::string key = "key" + std::to_string(i + 10);
      edgeId = wrapper.builder.addChild(rootNodeId, nodeId, key);
      expectedState[key].push(value);
    }
  }, wrapper.types);
  wrapper.types["type4"] = createTypeSpecFromRoot("type3",
  [&](const NodeId & rootNodeId, CoreTestWrapper & wrapper)
  {
    NodeId nodeId;
    EdgeId edgeId;

    expectedState2 = expectedState;

    auto children = wrapper.getMapNodeChildren(rootNodeId);
    for (int i = 0; i < 20; i++)
    {
      std::string key = "key" + std::to_string(i);
      wrapper.builder.removeChild(rootNodeId, children[key].first);
      expectedState2[key].pop();
      if (expectedState2[key].size() == 0)
      {
        expectedState2.erase(key);
      }
    }
  }, wrapper.types);

  NodeId mapNodeId = wrapper.builder.createNode("type3");
  wrapper.resolveTypes();

  std::unordered_map<std::string, int32_t> expected;
  std::unordered_map<std::string, int32_t> result;

  auto resultChildren = wrapper.getMapNodeChildren(mapNodeId);
  for (auto & kv : resultChildren)
  {
    result[kv.first] = wrapper.getNodeValue<int32_t>(kv.second.second);
  }
  for (auto & kv : expectedState)
  {
    expected[kv.first] = kv.second.top();
  }
  ASSERT_EQ(result, expected);

  expected.clear();
  result.clear();

  mapNodeId = wrapper.builder.createNode("type4");
  wrapper.resolveTypes();

  auto resultChildren2 = wrapper.getMapNodeChildren(mapNodeId);
  for (auto & kv : resultChildren2)
  {
    result[kv.first] = wrapper.getNodeValue<int32_t>(kv.second.second);
  }
  for (auto & kv : expectedState2)
  {
    expected[kv.first] = kv.second.top();
  }
  ASSERT_EQ(result, expected);
}

TEST(MapNodeTest, InsertSpeculativeNodeWorks)
{
  int eventCount = 0;

  CoreTestWrapper wrapper([&](const CoreTestWrapper & wrapper, const Event & event)
  {
    auto addedEvent = dynamic_cast<const NodeAddedEventMapped *>(&event);
    if (addedEvent != nullptr)
    {
      if (addedEvent->speculative)
      {
        EXPECT_EQ(addedEvent->childId, NodeId::Pending);
      }
      else
      {
        EXPECT_NE(addedEvent->childId, NodeId::Pending);
      }

      eventCount++;
    }

  });

  auto mapNodeId = wrapper.builder.createNode(PrimitiveNodeTypes::Map());

  auto childId0 = wrapper.builder.createNode("unresolved_type");
  auto edgeId0 = wrapper.builder.addChild(mapNodeId, childId0, "key");

  auto result = wrapper.getMapNodeChildren(mapNodeId, true);
  EXPECT_EQ(result.size(), 1);
  EXPECT_EQ(result["key"].first, edgeId0);
  EXPECT_EQ(result["key"].second, NodeId::Pending);

  auto childId1 = wrapper.builder.createNode(PrimitiveNodeTypes::Abstract());
  auto edgeId1 = wrapper.builder.addChild(mapNodeId, childId1, "key");

  result = wrapper.getMapNodeChildren(mapNodeId, true);
  EXPECT_EQ(result.size(), 1);
  EXPECT_EQ(result["key"].first, edgeId1);
  EXPECT_EQ(result["key"].second, childId1);
}