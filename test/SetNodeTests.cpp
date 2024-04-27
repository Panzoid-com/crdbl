#include <gtest/gtest.h>
#include <algorithm>
#include <random>
#include <Core.h>
#include "helpers.h"

TEST(SetNodeTest, InsertWorks)
{
  CoreTestWrapper wrapper;

  auto setNodeId = wrapper.builder.createNode(PrimitiveNodeTypes::Set());

  auto childId0 = wrapper.builder.createNode(PrimitiveNodeTypes::Abstract());
  auto edgeId0 = wrapper.builder.addChild(setNodeId, childId0);

  auto result = wrapper.getSetNodeChildren(setNodeId);
  ASSERT_EQ(result.size(), 1);
  ASSERT_EQ(result[0].first, edgeId0);
  ASSERT_EQ(result[0].second, childId0);
}

TEST(SetNodeTest, RemoveWorks)
{
  CoreTestWrapper wrapper;

  auto setNodeId = wrapper.builder.createNode(PrimitiveNodeTypes::Set());

  auto childId0 = wrapper.builder.createNode(PrimitiveNodeTypes::Abstract());
  auto edgeId0 = wrapper.builder.addChild(setNodeId, childId0);

  wrapper.builder.removeChild(setNodeId, edgeId0);

  auto result = wrapper.getSetNodeChildren(setNodeId);
  ASSERT_EQ(result.size(), 0);
}

TEST(SetNodeTest, InsertOrderIsCorrect)
{
  CoreTestWrapper wrapper;

  auto listNodeId = wrapper.builder.createNode(PrimitiveNodeTypes::Set());

  auto childId0 = wrapper.builder.createNode(PrimitiveNodeTypes::Abstract());
  auto edgeId0 = wrapper.builder.addChild(listNodeId, childId0);

  auto childId1 = wrapper.builder.createNode(PrimitiveNodeTypes::Abstract());
  auto edgeId1 = wrapper.builder.addChild(listNodeId, childId1);

  auto childId2 = wrapper.builder.createNode(PrimitiveNodeTypes::Abstract());
  auto edgeId2 = wrapper.builder.addChild(listNodeId, childId2);

  auto result = wrapper.getSetNodeChildren(listNodeId);
  ASSERT_EQ(result.size(), 3);
  ASSERT_EQ(result[0].first, edgeId2);
  ASSERT_EQ(result[0].second, childId2);
  ASSERT_EQ(result[1].first, edgeId1);
  ASSERT_EQ(result[1].second, childId1);
  ASSERT_EQ(result[2].first, edgeId0);
  ASSERT_EQ(result[2].second, childId0);
}

TEST(SetNodeTest, RandomOperationsTest)
{
  std::vector<std::pair<EdgeId, NodeId>> eventResult;

  CoreTestWrapper wrapper([&](const CoreTestWrapper & wrapper, const Event & event)
  {
    auto addedEvent = dynamic_cast<const NodeAddedEvent *>(&event);
    if (addedEvent != nullptr && addedEvent->parentId != NodeId::Root)
    {
      eventResult.insert(eventResult.begin(),
        std::make_pair(addedEvent->edgeId, addedEvent->childId));

      ASSERT_EQ(eventResult, wrapper.getSetNodeChildren(addedEvent->parentId));
      return;
    }

    auto removedEvent = dynamic_cast<const NodeRemovedEvent *>(&event);
    if (removedEvent != nullptr)
    {
      auto it = std::find_if(eventResult.begin(), eventResult.end(),
        [&](const std::pair<EdgeId, NodeId> & item)
        {
          return item.first == removedEvent->edgeId;
        });
      if (it != eventResult.end())
        eventResult.erase(it);

      ASSERT_EQ(eventResult, wrapper.getSetNodeChildren(removedEvent->parentId));
      return;
    }
  });

  auto setNodeId = wrapper.builder.createNode(PrimitiveNodeTypes::Set());

  std::vector<std::pair<EdgeId, NodeId>> expected;

  std::srand(0);
  const int numberOfOperations = 5000;
  for (int i = 0; i < numberOfOperations; i++)
  {
    if ((std::rand() & 3) != 0 || expected.size() == 0)
    {
      EdgeId edgeId;
      NodeId childId;

      int insertAfterIdx = std::rand() % (expected.size() + 1);
      if (insertAfterIdx == 0)
      {
        childId = wrapper.builder.createNode(PrimitiveNodeTypes::Abstract());
        edgeId = wrapper.builder.addChild(setNodeId, childId);
      }
      else
      {
        auto prevEdgeId = expected.at(insertAfterIdx - 1).first;
        childId = wrapper.builder.createNode(PrimitiveNodeTypes::Abstract());
        edgeId = wrapper.builder.addChild(setNodeId, childId);
      }

      expected.insert(expected.begin(), std::make_pair(edgeId, childId));
    }
    else
    {
      int deleteIdx = std::rand() % expected.size();
      wrapper.builder.removeChild(setNodeId, expected.at(deleteIdx).first);
      expected.erase(expected.begin() + deleteIdx);
    }
  }

  auto result = wrapper.getSetNodeChildren(setNodeId);
  ASSERT_EQ(result, expected);
  ASSERT_EQ(eventResult, expected);
}

TEST(SetNodeTest, InsertSpeculativeNodeWorks)
{
  CoreTestWrapper wrapper([&](const CoreTestWrapper & wrapper, const Event & event)
  {
    auto addedEvent = dynamic_cast<const NodeAddedEvent *>(&event);
    if (addedEvent != nullptr)
    {
      if (addedEvent->speculative)
      {
        ASSERT_EQ(addedEvent->childId, NodeId::Pending);
      }
    }
  });

  std::vector<std::pair<EdgeId, NodeId>> result;
  auto setNodeId = wrapper.builder.createNode(PrimitiveNodeTypes::Set());

  auto childId0 = wrapper.builder.createNode("unresolved_type");
  auto edgeId0 = wrapper.builder.addChild(setNodeId, childId0);

  result = wrapper.getSetNodeChildren(setNodeId, true);
  ASSERT_EQ(result.size(), 1);
  ASSERT_EQ(result[0].first, edgeId0);
  ASSERT_EQ(result[0].second, NodeId::Pending);

  auto childId1 = wrapper.builder.createNode(PrimitiveNodeTypes::Abstract());
  auto edgeId1 = wrapper.builder.addChild(setNodeId, childId1);

  result = wrapper.getSetNodeChildren(setNodeId, true);
  ASSERT_EQ(result.size(), 2);
  ASSERT_EQ(result[0].first, edgeId1);
  ASSERT_EQ(result[0].second, childId1);
}

TEST(SetNodeTest, ChildTypeAttributeEnforcesChildType)
{
  CoreTestWrapper wrapper;

  NodeType childType = PrimitiveNodeTypes::Int32Value();

  auto setNodeId = wrapper.builder.createContainerNode(PrimitiveNodeTypes::Set(),
    childType);

  wrapper.builder.addChild(setNodeId,
    wrapper.builder.createNode(PrimitiveNodeTypes::Abstract()));

  auto result0 = wrapper.getSetNodeChildren(setNodeId);
  ASSERT_EQ(result0.size(), 0);

  auto edgeId1 = wrapper.builder.addChild(setNodeId,
    wrapper.builder.createNode(childType));

  auto result1 = wrapper.getSetNodeChildren(setNodeId);
  ASSERT_EQ(result1.size(), 1);
  ASSERT_EQ(result1[0].first, edgeId1);
}

TEST(SetNodeTest, InheritanceWorks)
{
  CoreTestWrapper wrapper;

  std::vector<int32_t> expected;
  std::vector<int32_t> expected2;

  std::srand(0);

  wrapper.types["type0"] = createTypeSpec([](OperationBuilder & builder)
  {
    builder.createNode(PrimitiveNodeTypes::Set());
  }, wrapper.types);
  wrapper.types["type1"] = createTypeSpecFromRoot("type0",
  [&](const NodeId & rootNodeId, OperationBuilder & builder)
  {
    NodeId nodeId;
    EdgeId edgeId = EdgeId::Null;

    for (int i = 0; i < 10; i++)
    {
      nodeId = builder.createNode(PrimitiveNodeTypes::Int32Value());
      int32_t value = 100 + i;
      builder.setValue(nodeId, value);
      edgeId = builder.addChild(rootNodeId, nodeId);
      expected.insert(expected.begin(), value);
    }
  }, wrapper.types);
  wrapper.types["type2"] = createTypeSpecFromRoot("type1",
  [&](const NodeId & rootNodeId, OperationBuilder & builder)
  {
    NodeId nodeId;
    EdgeId edgeId;

    for (int i = 0; i < 10; i++)
    {
      nodeId = builder.createNode(PrimitiveNodeTypes::Int32Value());
      int32_t value = 200 + i;
      builder.setValue(nodeId, value);
      edgeId = builder.addChild(rootNodeId, nodeId);
      expected.insert(expected.begin(), value);
    }
  }, wrapper.types);
  wrapper.types["type3"] = createTypeSpecFromRoot("type2",
  [&](const NodeId & rootNodeId, OperationBuilder & builder)
  {
    NodeId nodeId;
    EdgeId edgeId;

    for (int i = 0; i < 10; i++)
    {
      size_t prevIdx = std::rand() % expected.size();
      nodeId = builder.createNode(PrimitiveNodeTypes::Int32Value());
      int32_t value = 300 + i;
      builder.setValue(nodeId, value);
      edgeId = builder.addChild(rootNodeId, nodeId);
      expected.insert(expected.begin(), value);
    }
  }, wrapper.types);
  wrapper.types["type4"] = createTypeSpec([](CoreTestWrapper & wrapper)
  {
    NodeId rootNodeId = wrapper.builder.createNode(PrimitiveNodeTypes::Set());
    wrapper.builder.addChild(rootNodeId, wrapper.builder.createNode("type3"));
  }, wrapper.types);
  wrapper.types["type5"] = createTypeSpec([&](CoreTestWrapper & wrapper)
  {
    NodeId rootNodeId = wrapper.builder.createNode("type4");
    wrapper.resolveTypes();
    auto children = wrapper.getSetNodeChildren(rootNodeId);
    ASSERT_GT(children.size(), 0);
    NodeId setNodeId = children[0].second;

    expected2 = expected;
    NodeId nodeId;
    EdgeId edgeId;

    for (int i = 0; i < 10; i++)
    {
      size_t prevIdx = std::rand() % expected2.size();
      nodeId = wrapper.builder.createNode(PrimitiveNodeTypes::Int32Value());
      int32_t value = 400 + i;
      wrapper.builder.setValue(nodeId, value);
      edgeId = wrapper.builder.addChild(setNodeId, nodeId);
      expected2.insert(expected2.begin(), value);
    }
  }, wrapper.types);

  NodeId setNodeId = wrapper.builder.createNode("type3");
  wrapper.resolveTypes();

  auto resultChildren = wrapper.getSetNodeChildren(setNodeId);
  std::vector<int32_t> result;
  for (int i = 0; i < resultChildren.size(); i++)
  {
    result.push_back(wrapper.getNodeValue<int32_t>(resultChildren[i].second));
  }
  ASSERT_EQ(result, expected);

  NodeId parentsetNodeId2 = wrapper.builder.createNode("type5");
  wrapper.resolveTypes();
  NodeId setNodeId2 = wrapper.getSetNodeChildren(parentsetNodeId2)[0].second;

  auto result2Children = wrapper.getSetNodeChildren(setNodeId2);
  std::vector<int32_t> result2;
  for (int i = 0; i < result2Children.size(); i++)
  {
    result2.push_back(wrapper.getNodeValue<int32_t>(result2Children[i].second));
  }
  ASSERT_EQ(result2, expected2);
}