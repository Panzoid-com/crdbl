#include <gtest/gtest.h>
#include <Core.h>
#include <PrimitiveNodeTypes.h>
#include "helpers.h"

template <typename T>
void testSetValue(NodeType nodeType, T value)
{
  CoreTestWrapper wrapper;
  NodeId valueNodeId = wrapper.builder.createNode(nodeType);
  wrapper.builder.setValue<T>(valueNodeId, value);
  ASSERT_EQ(value, wrapper.getNodeValue<T>(valueNodeId));
}

TEST(ValueTest, SetValueWorks)
{
  testSetValue<int32_t>(PrimitiveNodeTypes::Int32Value(), 42);
  testSetValue<int64_t>(PrimitiveNodeTypes::Int64Value(), 42);
  testSetValue<float>(PrimitiveNodeTypes::FloatValue(), 42.0f);
  testSetValue<double>(PrimitiveNodeTypes::DoubleValue(), 42.0);
  testSetValue<int8_t>(PrimitiveNodeTypes::Int8Value(), 42);
  testSetValue<bool>(PrimitiveNodeTypes::BoolValue(), true);
}

TEST(ValueTest, SetValueOverridesPrevious)
{
  CoreTestWrapper wrapper;
  NodeId valueNodeId = wrapper.builder.createNode(PrimitiveNodeTypes::Int32Value());

  int32_t value0 = 123;
  wrapper.builder.setValue<int32_t>(valueNodeId, value0);
  ASSERT_EQ(value0, wrapper.getNodeValue<int32_t>(valueNodeId));

  int32_t value1 = 234;
  ASSERT_NE(value0, value1);
  wrapper.builder.setValue<int32_t>(valueNodeId, value1);
  ASSERT_EQ(value1, wrapper.getNodeValue<int32_t>(valueNodeId));
}

TEST(ValueTest, ConcurrentSetValueIsConsistent)
{
  CoreTestWrapper wrapper1;
  wrapper1.builder.setSiteId(1);
  NodeId valueNodeId = wrapper1.builder.createNode(PrimitiveNodeTypes::Int32Value());

  CoreTestWrapper wrapper2;
  wrapper2.builder.setSiteId(2);
  wrapper2.applyOpsFrom(wrapper1);

  int32_t value0 = 123;
  wrapper1.builder.setValue<int32_t>(valueNodeId, value0);
  ASSERT_EQ(value0, wrapper1.getNodeValue<int32_t>(valueNodeId));

  int32_t value1 = 234;
  ASSERT_NE(value0, value1);
  Timestamp ts0 = wrapper2.builder.setValue<int32_t>(valueNodeId, value1);
  ASSERT_EQ(value1, wrapper2.getNodeValue<int32_t>(valueNodeId));

  wrapper1.applyOpsFrom(wrapper2);
  //higher siteId wins in concurrent edits
  ASSERT_EQ(value1, wrapper1.getNodeValue<int32_t>(valueNodeId));

  wrapper2.applyOpsFrom(wrapper1);
  ASSERT_EQ(value1, wrapper2.getNodeValue<int32_t>(valueNodeId));
}

TEST(ValueTest, SetValuePreviewWorks)
{
  CoreTestWrapper wrapper;
  NodeId valueNodeId = wrapper.builder.createNode(PrimitiveNodeTypes::Int32Value());

  int32_t initialValue = 42;
  wrapper.builder.setValue<int32_t>(valueNodeId, initialValue);

  auto values = { 1, 2, 3, 4, 5, 6, 7, 8, 9, 10 };
  for (int32_t value : values)
  {
    wrapper.builder.setValuePreview(valueNodeId,
      reinterpret_cast<const uint8_t *>(&value), sizeof(value));
    ASSERT_EQ(value, wrapper.getNodeValue<int32_t>(valueNodeId));
  }

  //reset to initial value by clearing the preview
  wrapper.builder.setValuePreview(valueNodeId, nullptr, 0);
  ASSERT_EQ(initialValue, wrapper.getNodeValue<int32_t>(valueNodeId));
}

TEST(ValueTest, UndoSetValueWorks)
{
  CoreTestWrapper wrapper;

  NodeId valueNodeId = wrapper.builder.createNode(PrimitiveNodeTypes::Int32Value());

  int32_t value0 = 123;
  wrapper.builder.setValue<int32_t>(valueNodeId, value0);
  ASSERT_EQ(value0, wrapper.getNodeValue<int32_t>(valueNodeId));

  int32_t value1 = 234;
  ASSERT_NE(value0, value1);
  Timestamp ts = wrapper.builder.setValue<int32_t>(valueNodeId, value1);
  ASSERT_EQ(value1, wrapper.getNodeValue<int32_t>(valueNodeId));

  undoOperation(wrapper, wrapper.log, ts);

  ASSERT_EQ(value0, wrapper.getNodeValue<int32_t>(valueNodeId));
}

TEST(ValueTest, RedoSetValueWorks)
{
  CoreTestWrapper wrapper;

  NodeId valueNodeId = wrapper.builder.createNode(PrimitiveNodeTypes::Int32Value());

  int32_t value0 = 123;
  wrapper.builder.setValue<int32_t>(valueNodeId, value0);
  ASSERT_EQ(value0, wrapper.getNodeValue<int32_t>(valueNodeId));

  int32_t value1 = 234;
  ASSERT_NE(value0, value1);
  Timestamp ts = wrapper.builder.setValue<int32_t>(valueNodeId, value1);
  ASSERT_EQ(value1, wrapper.getNodeValue<int32_t>(valueNodeId));

  Timestamp undoTs = undoOperation(wrapper, wrapper.log, ts);
  undoOperation(wrapper, wrapper.log, undoTs);

  ASSERT_EQ(value1, wrapper.getNodeValue<int32_t>(valueNodeId));
}

TEST(ValueTest, UnapplyAndReapplySetValueWorks)
{
  CoreTestWrapper wrapper;

  NodeId valueNodeId = wrapper.builder.createNode(PrimitiveNodeTypes::Int32Value());

  int32_t value0 = 123;
  wrapper.builder.setValue<int32_t>(valueNodeId, value0);
  ASSERT_EQ(value0, wrapper.getNodeValue<int32_t>(valueNodeId));

  int32_t value1 = 234;
  ASSERT_NE(value0, value1);
  Timestamp ts = wrapper.builder.setValue<int32_t>(valueNodeId, value1);
  ASSERT_EQ(value1, wrapper.getNodeValue<int32_t>(valueNodeId));

  unapplyOperation(wrapper, wrapper.log, ts);
  ASSERT_EQ(value0, wrapper.getNodeValue<int32_t>(valueNodeId));

  applyOperation(wrapper, wrapper.log, ts);
  ASSERT_EQ(value1, wrapper.getNodeValue<int32_t>(valueNodeId));
}

TEST(ValueTest, WrongValueSizeDoesNothing)
{
  CoreTestWrapper wrapper;

  NodeId valueNodeId = wrapper.builder.createNode(PrimitiveNodeTypes::Int32Value());

  int32_t value0 = 123;
  wrapper.builder.setValue<int32_t>(valueNodeId, value0);
  ASSERT_EQ(value0, wrapper.getNodeValue<int32_t>(valueNodeId));

  int8_t value1 = 127;
  ASSERT_NE(value0, value1);
  wrapper.builder.setValue<int8_t>(valueNodeId, value1);
  ASSERT_EQ(value0, wrapper.getNodeValue<int32_t>(valueNodeId));

  int64_t value2 = 345;
  ASSERT_NE(value0, value1);
  wrapper.builder.setValue<int64_t>(valueNodeId, value2);
  ASSERT_EQ(value0, wrapper.getNodeValue<int32_t>(valueNodeId));
}

TEST(ValueTest, SetValueEventRaisedWhenValueChanges)
{
  int32_t result = 0;
  int32_t eventCount = 0;

  CoreTestWrapper wrapper([&](const CoreTestWrapper & wrapper, const Event & event)
  {
    auto changeEvent = dynamic_cast<const NodeValueChangedEvent<int32_t> *>(&event);
    if (changeEvent != nullptr)
    {
      ASSERT_NE(changeEvent->newValue, changeEvent->oldValue);
      ASSERT_EQ(changeEvent->oldValue, result);
      ASSERT_NE(changeEvent->newValue, result);

      result = changeEvent->newValue;
      eventCount++;

      ASSERT_EQ(result, wrapper.getNodeValue<int32_t>(changeEvent->nodeId));
    }
  });
  NodeId valueNodeId = wrapper.builder.createNode(PrimitiveNodeTypes::Int32Value());

  int32_t value = 123;

  ASSERT_NE(value, result);
  wrapper.builder.setValue<int32_t>(valueNodeId, value);
  ASSERT_EQ(value, result);
  ASSERT_EQ(eventCount, 1);

  value = 234;

  ASSERT_NE(value, result);
  wrapper.builder.setValue<int32_t>(valueNodeId, value);
  ASSERT_EQ(value, result);
  ASSERT_EQ(eventCount, 2);
}

TEST(ValueTest, SetValueEventNotRaisedWhenValueDoesNotChange)
{
  int32_t result = 0;
  int32_t eventCount = 0;

  CoreTestWrapper wrapper([&](const CoreTestWrapper & wrapper, const Event & event)
  {
    auto changeEvent = dynamic_cast<const NodeValueChangedEvent<int32_t> *>(&event);
    if (changeEvent != nullptr)
    {
      ASSERT_NE(changeEvent->newValue, changeEvent->oldValue);
      ASSERT_EQ(changeEvent->oldValue, result);
      ASSERT_NE(changeEvent->newValue, result);

      result = changeEvent->newValue;
      eventCount++;

      ASSERT_EQ(result, wrapper.getNodeValue<int32_t>(changeEvent->nodeId));
    }
  });
  NodeId valueNodeId = wrapper.builder.createNode(PrimitiveNodeTypes::Int32Value());

  int32_t value = 123;
  ASSERT_NE(value, result);
  wrapper.builder.setValue<int32_t>(valueNodeId, value);
  ASSERT_EQ(value, result);
  ASSERT_EQ(eventCount, 1);

  ASSERT_EQ(value, result);
  wrapper.builder.setValue<int32_t>(valueNodeId, value);
  ASSERT_EQ(value, result);
  ASSERT_EQ(eventCount, 1);
}

TEST(ValueTest, SetValuePreviewEventsWork)
{
  int32_t result = 0;
  int eventCount = 0;
  int expectedEventCount = 0;

  CoreTestWrapper wrapper([&](const CoreTestWrapper & wrapper, const Event & event)
  {
    auto changeEvent = dynamic_cast<const NodeValueChangedEvent<int32_t> *>(&event);
    if (changeEvent != nullptr)
    {
      ASSERT_NE(changeEvent->newValue, changeEvent->oldValue);
      ASSERT_EQ(changeEvent->oldValue, result);
      ASSERT_NE(changeEvent->newValue, result);

      result = changeEvent->newValue;
      eventCount++;

      ASSERT_EQ(result, wrapper.getNodeValue<int32_t>(changeEvent->nodeId));
    }
  });
  NodeId valueNodeId = wrapper.builder.createNode(PrimitiveNodeTypes::Int32Value());

  int32_t initialValue = 123;
  ASSERT_NE(initialValue, result);
  wrapper.builder.setValue<int32_t>(valueNodeId, initialValue);
  expectedEventCount++;
  ASSERT_EQ(initialValue, result);
  ASSERT_EQ(eventCount, expectedEventCount);

  auto values = { 1, 2, 3, 4, 5, 6, 7, 8, 9, 10 };
  for (int32_t value : values)
  {
    wrapper.builder.setValuePreview(valueNodeId,
      reinterpret_cast<const uint8_t *>(&value), sizeof(value));
    expectedEventCount++;
    ASSERT_EQ(value, wrapper.getNodeValue<int32_t>(valueNodeId));
    ASSERT_EQ(eventCount, expectedEventCount);
  }

  wrapper.builder.setValuePreview(valueNodeId, nullptr, 0);
  expectedEventCount++;
  ASSERT_EQ(initialValue, wrapper.getNodeValue<int32_t>(valueNodeId));
  ASSERT_EQ(eventCount, expectedEventCount);
}

TEST(ValueTest, SetValueUndoEventsWork)
{
  int32_t result = 0;
  int32_t eventCount = 0;

  CoreTestWrapper wrapper([&](const CoreTestWrapper & wrapper, const Event & event)
  {
    auto changeEvent = dynamic_cast<const NodeValueChangedEvent<int32_t> *>(&event);
    if (changeEvent != nullptr)
    {
      ASSERT_NE(changeEvent->newValue, changeEvent->oldValue);
      ASSERT_EQ(changeEvent->oldValue, result);
      ASSERT_NE(changeEvent->newValue, result);

      result = changeEvent->newValue;
      eventCount++;

      ASSERT_EQ(result, wrapper.getNodeValue<int32_t>(changeEvent->nodeId));
    }
  });
  NodeId valueNodeId = wrapper.builder.createNode(PrimitiveNodeTypes::Int32Value());

  int32_t value0 = 123;
  wrapper.builder.setValue<int32_t>(valueNodeId, value0);
  ASSERT_EQ(value0, result);
  ASSERT_EQ(eventCount, 1);

  int32_t value1 = 234;
  ASSERT_NE(value0, value1);
  Timestamp ts = wrapper.builder.setValue<int32_t>(valueNodeId, value1);
  ASSERT_EQ(value1, result);
  ASSERT_EQ(eventCount, 2);

  Timestamp undoTs = undoOperation(wrapper, wrapper.log, ts);
  ASSERT_EQ(value0, result);
  ASSERT_EQ(eventCount, 3);

  undoOperation(wrapper, wrapper.log, undoTs);
  ASSERT_EQ(value1, result);
  ASSERT_EQ(eventCount, 4);
}

TEST(ValueTest, SetValueUnapplyEventsWork)
{
  int32_t result = 0;
  int32_t eventCount = 0;

  CoreTestWrapper wrapper([&](const CoreTestWrapper & wrapper, const Event & event)
  {
    auto changeEvent = dynamic_cast<const NodeValueChangedEvent<int32_t> *>(&event);
    if (changeEvent != nullptr)
    {
      ASSERT_NE(changeEvent->newValue, changeEvent->oldValue);
      ASSERT_EQ(changeEvent->oldValue, result);
      ASSERT_NE(changeEvent->newValue, result);

      result = changeEvent->newValue;
      eventCount++;

      ASSERT_EQ(result, wrapper.getNodeValue<int32_t>(changeEvent->nodeId));
    }
  });
  NodeId valueNodeId = wrapper.builder.createNode(PrimitiveNodeTypes::Int32Value());

  int32_t value0 = 123;
  wrapper.builder.setValue<int32_t>(valueNodeId, value0);
  ASSERT_EQ(value0, result);
  ASSERT_EQ(eventCount, 1);

  int32_t value1 = 234;
  ASSERT_NE(value0, value1);
  Timestamp ts = wrapper.builder.setValue<int32_t>(valueNodeId, value1);
  ASSERT_EQ(value1, result);
  ASSERT_EQ(eventCount, 2);

  unapplyOperation(wrapper, wrapper.log, ts);
  ASSERT_EQ(value0, result);
  ASSERT_EQ(eventCount, 3);

  applyOperation(wrapper, wrapper.log, ts);
  ASSERT_EQ(value1, result);
  ASSERT_EQ(eventCount, 4);
}