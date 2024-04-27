#include <gtest/gtest.h>
#include <algorithm>
#include <random>
#include <Core.h>
#include "helpers.h"

TEST(ListNodeTest, InsertWorks)
{
  CoreTestWrapper wrapper;

  auto listNodeId = wrapper.builder.createNode(PrimitiveNodeTypes::List());

  auto childId0 = wrapper.builder.createNode(PrimitiveNodeTypes::Abstract());
  auto edgeId0 = wrapper.builder.addChild(listNodeId, childId0,
    wrapper.builder.createPositionBetweenEdges(EdgeId::Null, EdgeId::Null));

  auto result = wrapper.getListNodeChildren(listNodeId);
  ASSERT_EQ(result.size(), 1);
  ASSERT_EQ(result[0].first, edgeId0);
  ASSERT_EQ(result[0].second, childId0);
}

TEST(ListNodeTest, RemoveWorks)
{
  CoreTestWrapper wrapper;

  auto listNodeId = wrapper.builder.createNode(PrimitiveNodeTypes::List());

  auto childId0 = wrapper.builder.createNode(PrimitiveNodeTypes::Abstract());
  auto edgeId0 = wrapper.builder.addChild(listNodeId, childId0,
    wrapper.builder.createPositionBetweenEdges(EdgeId::Null, EdgeId::Null));

  wrapper.builder.removeChild(listNodeId, edgeId0);

  auto result = wrapper.getListNodeChildren(listNodeId);
  ASSERT_EQ(result.size(), 0);
}

TEST(ListNodeTest, InsertAfterWorks)
{
  CoreTestWrapper wrapper;

  auto listNodeId = wrapper.builder.createNode(PrimitiveNodeTypes::List());

  auto childId0 = wrapper.builder.createNode(PrimitiveNodeTypes::Abstract());
  auto edgeId0 = wrapper.builder.addChild(listNodeId, childId0,
    wrapper.builder.createPositionBetweenEdges(EdgeId::Null, EdgeId::Null));

  auto childId1 = wrapper.builder.createNode(PrimitiveNodeTypes::Abstract());
  auto edgeId1 = wrapper.builder.addChild(listNodeId, childId1,
    wrapper.builder.createPositionBetweenEdges(edgeId0, EdgeId::Null));

  auto result = wrapper.getListNodeChildren(listNodeId);
  ASSERT_EQ(result.size(), 2);
  ASSERT_EQ(result[0].first, edgeId0);
  ASSERT_EQ(result[0].second, childId0);
  ASSERT_EQ(result[1].first, edgeId1);
  ASSERT_EQ(result[1].second, childId1);
}

TEST(ListNodeTest, InsertAtFrontWorks)
{
  CoreTestWrapper wrapper;

  auto listNodeId = wrapper.builder.createNode(PrimitiveNodeTypes::List());

  auto childId0 = wrapper.builder.createNode(PrimitiveNodeTypes::Abstract());
  auto edgeId0 = wrapper.builder.addChild(listNodeId, childId0,
    wrapper.builder.createPositionBetweenEdges(EdgeId::Null, EdgeId::Null));

  auto childId1 = wrapper.builder.createNode(PrimitiveNodeTypes::Abstract());
  auto edgeId1 = wrapper.builder.addChild(listNodeId, childId1,
    wrapper.builder.createPositionBetweenEdges(edgeId0, EdgeId::Null));

  auto childId2 = wrapper.builder.createNode(PrimitiveNodeTypes::Abstract());
  auto edgeId2 = wrapper.builder.addChild(listNodeId, childId2,
    wrapper.builder.createPositionBetweenEdges(EdgeId::Null, EdgeId::Null));

  auto result = wrapper.getListNodeChildren(listNodeId);
  ASSERT_EQ(result.size(), 3);
  ASSERT_EQ(result[0].first, edgeId2);
  ASSERT_EQ(result[0].second, childId2);
  ASSERT_EQ(result[1].first, edgeId0);
  ASSERT_EQ(result[1].second, childId0);
  ASSERT_EQ(result[2].first, edgeId1);
  ASSERT_EQ(result[2].second, childId1);
}

TEST(ListNodeTest, InsertSpeculativeNodeWorks)
{
  CoreTestWrapper wrapper([&](const CoreTestWrapper & wrapper, const Event & event)
  {
    auto addedEvent = dynamic_cast<const NodeAddedEventOrdered *>(&event);
    if (addedEvent != nullptr)
    {
      if (addedEvent->speculative)
      {
        ASSERT_EQ(addedEvent->childId, NodeId::Pending);
        ASSERT_EQ(addedEvent->actualIndex, 0);
        ASSERT_EQ(addedEvent->index, 0);
      }
      else
      {
        ASSERT_EQ(addedEvent->actualIndex, 1);
        ASSERT_EQ(addedEvent->index, 0);
      }
    }
  });

  std::vector<std::pair<EdgeId, NodeId>> result;
  auto listNodeId = wrapper.builder.createNode(PrimitiveNodeTypes::List());

  auto childId0 = wrapper.builder.createNode("unresolved_type");
  auto edgeId0 = wrapper.builder.addChild(listNodeId, childId0,
    wrapper.builder.createPositionBetweenEdges(EdgeId::Null, EdgeId::Null));

  result = wrapper.getListNodeChildren(listNodeId, true);
  ASSERT_EQ(result.size(), 1);
  ASSERT_EQ(result[0].first, edgeId0);
  ASSERT_EQ(result[0].second, NodeId::Pending);

  auto childId1 = wrapper.builder.createNode(PrimitiveNodeTypes::Abstract());
  auto edgeId1 = wrapper.builder.addChild(listNodeId, childId1,
    wrapper.builder.createPositionBetweenEdges(edgeId0, EdgeId::Null));

  result = wrapper.getListNodeChildren(listNodeId, true);
  ASSERT_EQ(result.size(), 2);
  ASSERT_EQ(result[1].first, edgeId1);
  ASSERT_EQ(result[1].second, childId1);
}

TEST(ListNodeTest, ChildTypeAttributeEnforcesChildType)
{
  CoreTestWrapper wrapper;

  NodeType childType = PrimitiveNodeTypes::Int32Value();

  auto listNodeId = wrapper.builder.createContainerNode(PrimitiveNodeTypes::List(),
    childType);

  wrapper.builder.addChild(listNodeId,
    wrapper.builder.createNode(PrimitiveNodeTypes::Abstract()),
    wrapper.builder.createPositionBetweenEdges(EdgeId::Null, EdgeId::Null));

  auto result0 = wrapper.getListNodeChildren(listNodeId);
  ASSERT_EQ(result0.size(), 0);

  auto edgeId1 = wrapper.builder.addChild(listNodeId,
    wrapper.builder.createNode(childType),
    wrapper.builder.createPositionBetweenEdges(EdgeId::Null, EdgeId::Null));

  auto result1 = wrapper.getListNodeChildren(listNodeId);
  ASSERT_EQ(result1.size(), 1);
  ASSERT_EQ(result1[0].first, edgeId1);
}

TEST(ListNodeTest, InheritanceWorks)
{
  CoreTestWrapper wrapper;

  std::vector<int32_t> expected;
  std::vector<int32_t> expected2;

  std::srand(0);

  wrapper.types["type0"] = createTypeSpec([](OperationBuilder & builder)
  {
    builder.createNode(PrimitiveNodeTypes::List());
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
      edgeId = builder.addChild(rootNodeId, nodeId,
        builder.createPositionBetweenEdges(edgeId, EdgeId::Null));
      expected.push_back(value);
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
      edgeId = builder.addChild(rootNodeId, nodeId,
        builder.createPositionFromIndex(rootNodeId, expected.size()));
      expected.push_back(value);
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
      edgeId = builder.addChild(rootNodeId, nodeId,
        builder.createPositionFromIndex(rootNodeId, prevIdx));
      expected.insert(expected.begin() + prevIdx, value);
    }
  }, wrapper.types);
  wrapper.types["type4"] = createTypeSpec([](CoreTestWrapper & wrapper)
  {
    NodeId rootNodeId = wrapper.builder.createNode("List");
    wrapper.builder.addChild(rootNodeId, wrapper.builder.createNode("type3"),
      wrapper.builder.createPositionBetweenEdges(EdgeId::Null, EdgeId::Null));
  }, wrapper.types);
  wrapper.types["type5"] = createTypeSpec([&](CoreTestWrapper & wrapper)
  {
    NodeId rootNodeId = wrapper.builder.createNode("type4");
    wrapper.resolveTypes();
    auto children = wrapper.getListNodeChildren(rootNodeId);
    ASSERT_GT(children.size(), 0);
    NodeId listNodeId = children[0].second;

    expected2 = expected;
    NodeId nodeId;
    EdgeId edgeId;

    for (int i = 0; i < 10; i++)
    {
      size_t prevIdx = std::rand() % expected2.size();
      nodeId = wrapper.builder.createNode(PrimitiveNodeTypes::Int32Value());
      int32_t value = 400 + i;
      wrapper.builder.setValue(nodeId, value);
      edgeId = wrapper.builder.addChild(listNodeId, nodeId,
        wrapper.builder.createPositionFromIndex(listNodeId, prevIdx));
      expected2.insert(expected2.begin() + prevIdx, value);
    }
  }, wrapper.types);

  NodeId listNodeId = wrapper.builder.createNode("type3");
  wrapper.resolveTypes();

  auto resultChildren = wrapper.getListNodeChildren(listNodeId);
  std::vector<int32_t> result;
  for (int i = 0; i < resultChildren.size(); i++)
  {
    result.push_back(wrapper.getNodeValue<int32_t>(resultChildren[i].second));
  }
  ASSERT_EQ(result, expected);

  NodeId parentListNodeId2 = wrapper.builder.createNode("type5");
  wrapper.resolveTypes();
  NodeId listNodeId2 = wrapper.getListNodeChildren(parentListNodeId2)[0].second;

  auto result2Children = wrapper.getListNodeChildren(listNodeId2);
  std::vector<int32_t> result2;
  for (int i = 0; i < result2Children.size(); i++)
  {
    result2.push_back(wrapper.getNodeValue<int32_t>(result2Children[i].second));
  }
  ASSERT_EQ(result2, expected2);
}

TEST(ListNodeTest, DoubleApplyDoesNothing)
{
  CoreTestWrapper wrapper;

  auto listId = wrapper.builder.createNode(PrimitiveNodeTypes::List());
  auto childId = wrapper.builder.createNode(PrimitiveNodeTypes::Abstract());
  auto edgeId = wrapper.builder.addChild(listId, childId,
    wrapper.builder.createPositionBetweenEdges(EdgeId::Null, EdgeId::Null));

  ASSERT_EQ(wrapper.getListNodeChildren(listId)[0].second, childId);

  applyOperation(wrapper, wrapper.log, edgeId.ts);
  ASSERT_EQ(wrapper.getListNodeChildren(listId)[0].second, childId);

  unapplyOperation(wrapper, wrapper.log, edgeId.ts);
  ASSERT_EQ(wrapper.getListNodeChildren(listId).size(), 0);

  applyOperation(wrapper, wrapper.log, edgeId.ts);
  ASSERT_EQ(wrapper.getListNodeChildren(listId)[0].second, childId);
}

TEST(ListNodeTest, DoubleUnapplyDoesNothing)
{
  CoreTestWrapper wrapper;

  auto listId = wrapper.builder.createNode(PrimitiveNodeTypes::List());
  auto childId = wrapper.builder.createNode(PrimitiveNodeTypes::Abstract());
  auto edgeId = wrapper.builder.addChild(listId, childId,
    wrapper.builder.createPositionBetweenEdges(EdgeId::Null, EdgeId::Null));

  ASSERT_EQ(wrapper.getListNodeChildren(listId).size(), 1);
  ASSERT_EQ(wrapper.getListNodeChildren(listId)[0].second, childId);

  unapplyOperation(wrapper, wrapper.log, edgeId.ts);
  ASSERT_EQ(wrapper.getListNodeChildren(listId).size(), 0);

  unapplyOperation(wrapper, wrapper.log, edgeId.ts);
  ASSERT_EQ(wrapper.getListNodeChildren(listId).size(), 0);

  applyOperation(wrapper, wrapper.log, edgeId.ts);
  ASSERT_EQ(wrapper.getListNodeChildren(listId).size(), 1);
  ASSERT_EQ(wrapper.getListNodeChildren(listId)[0].second, childId);
}

TEST(ListNodeTest, UnapplyAndApplyPreservesDeletions)
{
  CoreTestWrapper wrapper;

  wrapper.builder.setSiteId(1);
  auto listId = wrapper.builder.createNode(PrimitiveNodeTypes::List());
  auto childId = wrapper.builder.createNode(PrimitiveNodeTypes::Abstract());
  auto edgeId = wrapper.builder.addChild(listId, childId,
    wrapper.builder.createPositionBetweenEdges(EdgeId::Null, EdgeId::Null));

  wrapper.builder.setSiteId(2);
  auto removeTs = wrapper.builder.removeChild(listId, edgeId);

  OperationFilter filter;
  filter.setSiteFilter(1);

  auto listNode = reinterpret_cast<const ListNode *>(wrapper.core->getExistingNode(listId));

  auto children = wrapper.getListNodeChildren(listId);
  ASSERT_EQ(children.size(), 0);

  unapplyOperations(wrapper, wrapper.log, filter);

  children = wrapper.getListNodeChildren(listId);
  ASSERT_EQ(children.size(), 0);

  applyOperations(wrapper, wrapper.log, filter);

  children = wrapper.getListNodeChildren(listId);
  ASSERT_EQ(children.size(), 0);
}

TEST(ListNodeTest, UnapplyAndApplyPreservesOrder)
{
  CoreTestWrapper wrapper;

  auto listId = wrapper.builder.createNode(PrimitiveNodeTypes::List());
  auto childId1 = wrapper.builder.createNode(PrimitiveNodeTypes::Abstract());
  auto edgeId1 = wrapper.builder.addChild(listId, childId1,
    wrapper.builder.createPositionBetweenEdges(EdgeId::Null, EdgeId::Null));
  auto childId2 = wrapper.builder.createNode(PrimitiveNodeTypes::Abstract());
  auto edgeId2 = wrapper.builder.addChild(listId, childId2,
    wrapper.builder.createPositionBetweenEdges(edgeId1, EdgeId::Null));
  auto childId3 = wrapper.builder.createNode(PrimitiveNodeTypes::Abstract());
  auto edgeId3 = wrapper.builder.addChild(listId, childId3,
    wrapper.builder.createPositionBetweenEdges(edgeId2, EdgeId::Null));

  auto listNode = reinterpret_cast<const ListNode *>(wrapper.core->getExistingNode(listId));

  auto childrenBefore = wrapper.getListNodeChildren(listId);
  ASSERT_EQ(childrenBefore.size(), 3);

  unapplyOperation(wrapper, wrapper.log, edgeId2.ts);

  auto childrenMiddle = wrapper.getListNodeChildren(listId);
  ASSERT_EQ(childrenMiddle.size(), 2);

  applyOperation(wrapper, wrapper.log, edgeId2.ts);

  auto childrenAfter = wrapper.getListNodeChildren(listId);
  ASSERT_EQ(childrenBefore, childrenAfter);
}

TEST(ListNodeTest, GeneralFuzz)
{
  std::vector<std::pair<EdgeId, NodeId>> eventResult;

  CoreTestWrapper wrapper([&](const CoreTestWrapper & wrapper, const Event & event)
  {
    auto addedEvent = dynamic_cast<const NodeAddedEventOrdered *>(&event);
    if (addedEvent != nullptr)
    {
      eventResult.insert(eventResult.begin() + addedEvent->actualIndex,
        std::make_pair(addedEvent->edgeId, addedEvent->childId));

      ASSERT_EQ(eventResult, wrapper.getListNodeChildren(addedEvent->parentId));
      return;
    }

    auto removedEvent = dynamic_cast<const NodeRemovedEventOrdered *>(&event);
    if (removedEvent != nullptr)
    {
      eventResult.erase(eventResult.begin() + removedEvent->actualIndex);

      ASSERT_EQ(eventResult, wrapper.getListNodeChildren(removedEvent->parentId));
      return;
    }
  });

  auto listNodeId = wrapper.builder.createNode(PrimitiveNodeTypes::List());

  std::vector<std::pair<EdgeId, NodeId>> expected;

  std::srand(0);
  const int numberOfOperations = 5000;
  for (int i = 0; i < numberOfOperations; i++)
  {
    if ((std::rand() & 3) != 0 || expected.size() == 0)
    {
      EdgeId edgeId;
      NodeId childId;
      std::vector<std::pair<EdgeId, NodeId>>::iterator insertIt;

      int insertAfterIdx = std::rand() % (expected.size() + 1);
      if (insertAfterIdx == 0)
      {
        childId = wrapper.builder.createNode(PrimitiveNodeTypes::Abstract());
        edgeId = wrapper.builder.addChild(listNodeId, childId,
          wrapper.builder.createPositionBetweenEdges(EdgeId::Null, EdgeId::Null));
        insertIt = expected.begin();
      }
      else
      {
        auto prevEdgeId = expected.at(insertAfterIdx - 1).first;
        childId = wrapper.builder.createNode(PrimitiveNodeTypes::Abstract());
        edgeId = wrapper.builder.addChild(listNodeId, childId,
          wrapper.builder.createPositionBetweenEdges(prevEdgeId, EdgeId::Null));
      }

      insertIt = (insertAfterIdx < expected.size())
        ? expected.begin() + insertAfterIdx : expected.end();
      expected.insert(insertIt, std::make_pair(edgeId, childId));
    }
    else
    {
      int deleteIdx = std::rand() % expected.size();
      wrapper.builder.removeChild(listNodeId, expected.at(deleteIdx).first);
      expected.erase(expected.begin() + deleteIdx);
    }
  }

  auto result = wrapper.getListNodeChildren(listNodeId);
  ASSERT_EQ(result, expected);
  ASSERT_EQ(eventResult, expected);
}