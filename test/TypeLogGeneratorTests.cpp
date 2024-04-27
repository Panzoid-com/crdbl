#include <gtest/gtest.h>
#include <algorithm>
#include <random>
#include <Core.h>
#include <TypeLogGenerator.h>
#include <Serialization/LogOperationSerialization.h>
#include <Streams/CallbackWritableStream.h>
#include "helpers.h"

void compareData(const CoreTestWrapper & wrapper1, NodeId firstNodeId,
  const CoreTestWrapper & wrapper2, NodeId secondNodeId)
{
  auto node1 = wrapper1.core->getExistingNode(firstNodeId);
  auto node2 = wrapper2.core->getExistingNode(secondNodeId);

  ASSERT_EQ(node1 == nullptr, node2 == nullptr);
  ASSERT_EQ(node1->getType(), node2->getType());

  if (node1->getBaseType() == PrimitiveNodeTypes::Map())
  {
    auto children1 = wrapper1.getMapNodeChildren(firstNodeId);
    auto children2 = wrapper2.getMapNodeChildren(secondNodeId);

    ASSERT_EQ(children1.size(), children2.size());

    for (auto & entry : children1)
    {
      auto it = children2.find(entry.first);
      if (it == children2.end())
      {
        FAIL() << "node1 map key " << entry.first << " not found in node2 map";
      }

      compareData(wrapper1, entry.second.second, wrapper2, it->second.second);
    }
  }
  else if (node1->getBaseType() == PrimitiveNodeTypes::List())
  {
    auto children1 = wrapper1.getListNodeChildren(firstNodeId);
    auto children2 = wrapper2.getListNodeChildren(secondNodeId);

    ASSERT_EQ(children1.size(), children2.size());

    for (size_t i = 0; i < children1.size(); i++)
    {
      compareData(wrapper1, children1[i].second, wrapper2, children2[i].second);
    }
  }
  else if (node1->getBaseType() == PrimitiveNodeTypes::Reference())
  {
    auto children1 = wrapper1.getReferenceNodeChildren(firstNodeId);
    auto children2 = wrapper2.getReferenceNodeChildren(secondNodeId);

    ASSERT_EQ(children1.size(), children2.size());

    for (size_t i = 0; i < children1.size(); i++)
    {
      compareData(wrapper1, children1[i].second, wrapper2, children2[i].second);
    }
  }
  else if (node1->getBaseType() == PrimitiveNodeTypes::Set())
  {
    auto children1 = wrapper1.getSetNodeChildren(firstNodeId);
    auto children2 = wrapper2.getSetNodeChildren(secondNodeId);

    ASSERT_EQ(children1.size(), children2.size());

    //NOTE: set comparison is non-trivial
    //  node ids are not necessarily the same and no order is required
    //    the only way to truly compare is value-wise
    //  for now, the size is decently sufficient to check if things are working
  }
  else if (node1->getBaseType() == PrimitiveNodeTypes::StringValue())
  {
    ASSERT_EQ(wrapper1.getNodeBlockValue(firstNodeId), wrapper2.getNodeBlockValue(secondNodeId));
  }
  else if (node1->getBaseType() == PrimitiveNodeTypes::DoubleValue())
  {
    ASSERT_EQ(wrapper1.getNodeValue<double>(firstNodeId), wrapper2.getNodeValue<double>(secondNodeId));
  }
  else if (node1->getBaseType() == PrimitiveNodeTypes::Int32Value())
  {
    ASSERT_EQ(wrapper1.getNodeValue<int>(firstNodeId), wrapper2.getNodeValue<int>(secondNodeId));
  }
  else if (node1->getBaseType() == PrimitiveNodeTypes::BoolValue())
  {
    ASSERT_EQ(wrapper1.getNodeValue<bool>(firstNodeId), wrapper2.getNodeValue<bool>(secondNodeId));
  }
  else
  {
    FAIL() << "node1 type " << node1->getBaseType().toString() << " comparison is not supported";
  }
}

TEST(TypeLogGeneratorTest, BasicDataTransferWorks)
{
  CoreTestWrapper wrapper;
  CoreTestWrapper wrapper2;

  NodeId parentId = wrapper.builder.createNode(PrimitiveNodeTypes::Map());
  NodeId childId = wrapper.builder.createNode(PrimitiveNodeTypes::StringValue());
  wrapper.builder.insertText(childId, 0, "test string");
  wrapper.builder.addChild(parentId, childId, "key");
  NodeId childId2 = wrapper.builder.createNode(PrimitiveNodeTypes::Int32Value());
  wrapper.builder.setValue<int>(childId2, 123);
  wrapper.builder.addChild(parentId, childId2, "key2");

  wrapper.resolveTypes();

  auto applyStream = wrapper2.builder.createApplyStream();
  TypeLogGenerator generator(wrapper.core);
  generator.addAllNodes(parentId);
  generator.generate(*applyStream);
  applyStream->close();
  delete applyStream;

  wrapper2.resolveTypes();

  compareData(wrapper, parentId, wrapper2, parentId);
}

TEST(TypeLogGeneratorTest, DataTransferWithInheritedTypesWorks)
{
  CoreTestWrapper wrapper;
  CoreTestWrapper wrapper2;
  CoreTestWrapper wrapper3;

  wrapper.types["type0"] = wrapper2.types["type0"] = wrapper3.types["type0"] =
  createTypeSpec([](CoreTestWrapper & wrapper)
  {
    NodeId rootId = wrapper.builder.createNode(PrimitiveNodeTypes::Map());
    NodeId stringId = wrapper.builder.createNode(PrimitiveNodeTypes::StringValue());
    wrapper.builder.insertText(stringId, 0, "test string");
    NodeId intId = wrapper.builder.createNode(PrimitiveNodeTypes::Int32Value());
    wrapper.builder.setValue<int>(intId, 123);

    wrapper.builder.addChild(rootId, stringId, "key");
    wrapper.builder.addChild(rootId, intId, "key2");
  }, wrapper.types);

  wrapper.types["type1"] = wrapper2.types["type1"] = wrapper3.types["type1"] =
  createTypeSpec([](CoreTestWrapper & wrapper)
  {
    NodeId rootId = wrapper.builder.createNode(PrimitiveNodeTypes::Map());
    NodeId childId = wrapper.builder.createNode("type0");
    wrapper.builder.addChild(rootId, childId, "key");
  }, wrapper.types);

  NodeId parentId = wrapper.builder.createNode("type0");
  NodeId secondParentId = wrapper.builder.createNode("type0");
  wrapper.resolveTypes();

  NodeId stringId = wrapper.getMapNodeChildren(parentId)["key"].second;
  wrapper.builder.deleteText(stringId, 0, std::string("test string").size());
  wrapper.builder.insertText(stringId, 0, "a different test string");
  NodeId intId = wrapper.getMapNodeChildren(parentId)["key2"].second;
  wrapper.builder.setValue<int>(intId, 456);

  wrapper.builder.addChild(parentId, secondParentId, "key3");
  NodeId stringId2 = wrapper.getMapNodeChildren(secondParentId)["key"].second;
  wrapper.builder.deleteText(stringId2, 0, std::string("test string").size());
  wrapper.builder.insertText(stringId2, 0, "a different test string");
  NodeId intId2 = wrapper.getMapNodeChildren(secondParentId)["key2"].second;
  wrapper.builder.setValue<int>(intId2, 456);

  wrapper.resolveTypes();

  auto applyStream = wrapper2.builder.createApplyStream();
  TypeLogGenerator generator(wrapper.core);
  generator.addAllNodes(parentId);
  generator.generate(*applyStream);
  applyStream->close();
  delete applyStream;

  wrapper2.resolveTypes();

  compareData(wrapper, parentId, wrapper2, parentId);

  auto applyStream2 = wrapper3.builder.createApplyStream();
  TypeLogGenerator generator2(wrapper2.core);
  generator2.addAllNodes(parentId);
  generator2.generate(*applyStream2);
  applyStream2->close();
  delete applyStream2;

  wrapper3.resolveTypes();

  compareData(wrapper, parentId, wrapper3, parentId);
}

TEST(TypeLogGeneratorTest, SpeculativeChildrenAreIgnored)
{
  CoreTestWrapper wrapper;
  CoreTestWrapper wrapper2;

  NodeId parentId = wrapper.builder.createNode(PrimitiveNodeTypes::Map());
  NodeId speculativeId = wrapper.builder.createNode("type0");
  NodeId childId = wrapper.builder.createNode(PrimitiveNodeTypes::Int32Value());

  NodeId mapId = wrapper.builder.createNode(PrimitiveNodeTypes::Map());
  wrapper.builder.addChild(mapId, childId, "key");
  wrapper.builder.addChild(mapId, speculativeId, "key");
  wrapper.builder.addChild(parentId, mapId, "mapNode");

  NodeId listId = wrapper.builder.createNode(PrimitiveNodeTypes::List());
  wrapper.builder.addChild(listId, childId,
    wrapper.builder.createPositionBetweenEdges(EdgeId::Null, EdgeId::Null));
  wrapper.builder.addChild(listId, speculativeId,
    wrapper.builder.createPositionBetweenEdges(EdgeId::Null, EdgeId::Null));
  wrapper.builder.addChild(parentId, listId, "listNode");

  NodeId referenceId = wrapper.builder.createNode(PrimitiveNodeTypes::Reference());
  wrapper.builder.addChild(referenceId, childId);
  wrapper.builder.addChild(referenceId, speculativeId);
  wrapper.builder.addChild(parentId, referenceId, "referenceNode");

  NodeId setId = wrapper.builder.createNode(PrimitiveNodeTypes::Set());
  wrapper.builder.addChild(setId, childId);
  wrapper.builder.addChild(setId, speculativeId);
  wrapper.builder.addChild(parentId, setId, "setNode");

  auto applyStream = wrapper2.builder.createApplyStream();
  TypeLogGenerator generator(wrapper.core);
  generator.addAllNodes(parentId);
  generator.generate(*applyStream);
  applyStream->close();
  delete applyStream;

  wrapper2.resolveTypes();

  compareData(wrapper, parentId, wrapper2, NodeId::SiteRoot);
}

// Generating a type log for an inherited node may require data that is
//   inherited by a type of its root node, which might not be created in the log
// In this case, the generator needs to explicitly include the missing data
TEST(TypeLogGeneratorTest, MissingInheritedDataIsIncluded)
{
  CoreTestWrapper wrapper;
  CoreTestWrapper wrapper2;

  wrapper.types["type0"] = wrapper2.types["type0"] =
  createTypeSpec([](CoreTestWrapper & wrapper)
  {
    NodeId rootId = wrapper.builder.createNode(PrimitiveNodeTypes::Map());

    NodeId stringId = wrapper.builder.createNode(PrimitiveNodeTypes::StringValue());
    wrapper.builder.insertText(stringId, 0, "test string");
    wrapper.builder.addChild(rootId, stringId, "stringNode");

    NodeId intId = wrapper.builder.createNode(PrimitiveNodeTypes::Int32Value());
    wrapper.builder.setValue<int>(intId, 123);
    wrapper.builder.addChild(rootId, intId, "valueNode");

    NodeId listId = wrapper.builder.createNode(PrimitiveNodeTypes::List());
    EdgeId prevEdge;
    for (int i = 0; i < 3; i++)
    {
      NodeId childId = wrapper.builder.createNode(PrimitiveNodeTypes::Int32Value());
      wrapper.builder.setValue<int>(childId, i);
      wrapper.builder.addChild(listId, childId,
        wrapper.builder.createPositionBetweenEdges(prevEdge, EdgeId::Null));
    }
    wrapper.builder.addChild(rootId, listId, "listNode");

    NodeId refId = wrapper.builder.createNode(PrimitiveNodeTypes::Reference());
    wrapper.builder.addChild(refId,
      wrapper.builder.createNode(PrimitiveNodeTypes::Int32Value()));
    wrapper.builder.addChild(rootId, refId, "refNode");

    NodeId mapId = wrapper.builder.createNode(PrimitiveNodeTypes::Map());
    for (int i = 0; i < 3; i++)
    {
      NodeId childId = wrapper.builder.createNode(PrimitiveNodeTypes::Int32Value());
      wrapper.builder.setValue<int>(childId, i);
      wrapper.builder.addChild(mapId, childId, std::to_string(i));
    }
    wrapper.builder.addChild(rootId, mapId, "mapNode");

    NodeId setId = wrapper.builder.createNode(PrimitiveNodeTypes::Set());
    for (int i = 0; i < 3; i++)
    {
      NodeId childId = wrapper.builder.createNode(PrimitiveNodeTypes::Int32Value());
      wrapper.builder.setValue<int>(childId, i);
      wrapper.builder.addChild(setId, childId);
    }
    wrapper.builder.addChild(rootId, setId, "setNode");
  }, wrapper.types);

  wrapper.types["type1"] = wrapper2.types["type1"] =
  createTypeSpec([](CoreTestWrapper & wrapper)
  {
    NodeId rootId = wrapper.builder.createNode(PrimitiveNodeTypes::Map());
    NodeId childId = wrapper.builder.createNode("type0");
    wrapper.builder.addChild(rootId, childId, "key");
    wrapper.resolveTypes();

    //modify the data inherited from type0, but in type1's context

    NodeId stringId = wrapper.getMapNodeChildren(childId)["stringNode"].second;
    wrapper.builder.deleteText(stringId, 0, std::string("test string").size());
    wrapper.builder.insertText(stringId, 0, "a different test string");

    NodeId intId = wrapper.getMapNodeChildren(childId)["valueNode"].second;
    wrapper.builder.setValue<int>(intId, 456);

    NodeId listId = wrapper.getMapNodeChildren(childId)["listNode"].second;
    auto children = wrapper.getListNodeChildren(listId);
    for (int i = 0; i < 3; i++)
    {
      NodeId childId = wrapper.builder.createNode(PrimitiveNodeTypes::Int32Value());
      wrapper.builder.setValue<int>(childId, i * 2);
      wrapper.builder.addChild(listId, childId,
        wrapper.builder.createPositionBetweenEdges(children[i].first, EdgeId::Null));
    }

    NodeId refId = wrapper.getMapNodeChildren(childId)["refNode"].second;
    NodeId newRefChild = wrapper.builder.createNode(PrimitiveNodeTypes::Int32Value());
    wrapper.builder.addChild(refId, newRefChild);
    wrapper.builder.setValue<int>(newRefChild, 789);

    NodeId mapId = wrapper.getMapNodeChildren(childId)["mapNode"].second;
    auto mapChildren = wrapper.getMapNodeChildren(mapId);
    for (int i = 0; i < 3; i++)
    {
      NodeId childId = mapChildren[std::to_string(i)].second;
      NodeId newChildId = wrapper.builder.createNode(PrimitiveNodeTypes::Int32Value());
      wrapper.builder.setValue<int>(newChildId, i * 3);
      wrapper.builder.addChild(mapId, newChildId, std::to_string(i));
    }

    NodeId setId = wrapper.getMapNodeChildren(childId)["setNode"].second;
    auto setChildren = wrapper.getSetNodeChildren(setId);
    for (int i = 3; i < 6; i++)
    {
      NodeId childId = wrapper.builder.createNode(PrimitiveNodeTypes::Int32Value());
      wrapper.builder.setValue<int>(childId, i * 4);
      wrapper.builder.addChild(setId, childId);
    }
  }, wrapper.types);

  NodeId parentId = wrapper.builder.createNode("type1");
  wrapper.resolveTypes();

  NodeId childId = wrapper.getMapNodeChildren(parentId)["key"].second;
  ASSERT_TRUE(childId.isInherited());

  auto applyStream = wrapper2.builder.createApplyStream();
  TypeLogGenerator generator(wrapper.core);
  generator.addAllNodes(childId);
  generator.generate(*applyStream);
  applyStream->close();
  delete applyStream;

  wrapper2.resolveTypes();

  compareData(wrapper, childId, wrapper2, NodeId::SiteRoot);
}