#include <gtest/gtest.h>
#include <algorithm>
#include <random>
#include <Core.h>
#include "helpers.h"

TEST(BlockValueTest, InsertWorks)
{
  CoreTestWrapper wrapper;

  std::string expected;
  std::string result;

  NodeId stringNodeId = wrapper.builder.createNode(PrimitiveNodeTypes::StringValue());

  std::string value0 = "Test insert value";
  wrapper.builder.insertText(stringNodeId, 0, value0);
  result = static_cast<const BlockValueNode<char> *>(wrapper.core->getExistingNode(stringNodeId))->value.toString();
  expected = value0;
  ASSERT_STREQ(expected.data(), result.data());

  std::string value1 = "Insert at beginning ";
  wrapper.builder.insertText(stringNodeId, 0, value1);
  result = static_cast<const BlockValueNode<char> *>(wrapper.core->getExistingNode(stringNodeId))->value.toString();
  expected.insert(0, value1);
  ASSERT_STREQ(expected.data(), result.data());

  std::string value2 = " Insert at end";
  wrapper.builder.insertText(stringNodeId, expected.length(), value2);
  result = static_cast<const BlockValueNode<char> *>(wrapper.core->getExistingNode(stringNodeId))->value.toString();
  expected.insert(expected.length(), value2);
  ASSERT_STREQ(expected.data(), result.data());

  std::string value3 = " Insert in the middle ";
  wrapper.builder.insertText(stringNodeId, value1.length() + 1, value3);
  result = static_cast<const BlockValueNode<char> *>(wrapper.core->getExistingNode(stringNodeId))->value.toString();
  expected.insert(value1.length() + 1, value3);
  ASSERT_STREQ(expected.data(), result.data());
}

TEST(BlockValueTest, DeleteWorks)
{
  CoreTestWrapper wrapper;

  std::string expected;
  std::string result;

  NodeId stringNodeId = wrapper.builder.createNode(PrimitiveNodeTypes::StringValue());

  std::string value0 = "Test insert value ";
  wrapper.builder.insertText(stringNodeId, 0, value0);
  result = static_cast<const BlockValueNode<char> *>(wrapper.core->getExistingNode(stringNodeId))->value.toString();
  expected = value0;
  ASSERT_STREQ(expected.data(), result.data());

  std::string value1 = "Test insert value ";
  wrapper.builder.insertText(stringNodeId, expected.length(), value1);
  result = static_cast<const BlockValueNode<char> *>(wrapper.core->getExistingNode(stringNodeId))->value.toString();
  expected.insert(expected.length(), value1);
  ASSERT_STREQ(expected.data(), result.data());

  std::string value2 = "Test insert value ";
  wrapper.builder.insertText(stringNodeId, expected.length(), value2);
  result = static_cast<const BlockValueNode<char> *>(wrapper.core->getExistingNode(stringNodeId))->value.toString();
  expected.insert(expected.length(), value2);
  ASSERT_STREQ(expected.data(), result.data());

  wrapper.builder.deleteText(stringNodeId, 0, 5);
  result = static_cast<const BlockValueNode<char> *>(wrapper.core->getExistingNode(stringNodeId))->value.toString();
  expected.erase(0, 5);
  ASSERT_STREQ(expected.data(), result.data());

  wrapper.builder.deleteText(stringNodeId, 5, 30);
  result = static_cast<const BlockValueNode<char> *>(wrapper.core->getExistingNode(stringNodeId))->value.toString();
  expected.erase(5, 30);
  ASSERT_STREQ(expected.data(), result.data());

  wrapper.builder.deleteText(stringNodeId, expected.length() - 5, 5);
  result = static_cast<const BlockValueNode<char> *>(wrapper.core->getExistingNode(stringNodeId))->value.toString();
  expected.erase(expected.length() - 5, 5);
  ASSERT_STREQ(expected.data(), result.data());

  std::string value3 = "Test insert value ";
  wrapper.builder.insertText(stringNodeId, 5, value3);
  result = static_cast<const BlockValueNode<char> *>(wrapper.core->getExistingNode(stringNodeId))->value.toString();
  expected.insert(5, value3);
  ASSERT_STREQ(expected.data(), result.data());

  std::string value4 = "Test insert value ";
  wrapper.builder.insertText(stringNodeId, 15, value4);
  result = static_cast<const BlockValueNode<char> *>(wrapper.core->getExistingNode(stringNodeId))->value.toString();
  expected.insert(15, value4);
  ASSERT_STREQ(expected.data(), result.data());

  wrapper.builder.deleteText(stringNodeId, 0, expected.length());
  result = static_cast<const BlockValueNode<char> *>(wrapper.core->getExistingNode(stringNodeId))->value.toString();
  expected.erase(0, expected.length());
  ASSERT_STREQ(expected.data(), result.data());
}

TEST(BlockValueTest, ConcurrentInsertIsConsistent)
{
  CoreTestWrapper wrapper1;
  CoreTestWrapper wrapper2;
  wrapper2.builder.setSiteId(2);

  NodeId stringNodeId = wrapper1.builder.createNode(PrimitiveNodeTypes::StringValue());
  wrapper2.applyOpsFrom(wrapper1);

  wrapper1.builder.insertText(stringNodeId, 0, "Test insert value");
  wrapper2.builder.insertText(stringNodeId, 0, "Insert at beginning ");

  wrapper1.applyOpsFrom(wrapper2);
  wrapper2.applyOpsFrom(wrapper1);

  ASSERT_EQ(wrapper1.getNodeBlockValue(stringNodeId), "Insert at beginning Test insert value");
  ASSERT_EQ(wrapper2.getNodeBlockValue(stringNodeId), "Insert at beginning Test insert value");

  wrapper1.builder.insertText(stringNodeId, 6, " text");
  wrapper2.builder.insertText(stringNodeId, 19, " in the middle");

  wrapper1.applyOpsFrom(wrapper2);
  wrapper2.applyOpsFrom(wrapper1);

  ASSERT_EQ(wrapper1.getNodeBlockValue(stringNodeId), "Insert text at beginning in the middle Test insert value");
  ASSERT_EQ(wrapper2.getNodeBlockValue(stringNodeId), "Insert text at beginning in the middle Test insert value");
}

TEST(BlockValueTest, ConcurrentDeleteIsConsistent)
{
  CoreTestWrapper wrapper1;
  CoreTestWrapper wrapper2;
  wrapper2.builder.setSiteId(2);

  NodeId stringNodeId = wrapper1.builder.createNode(PrimitiveNodeTypes::StringValue());
  wrapper2.applyOpsFrom(wrapper1);

  wrapper1.builder.insertText(stringNodeId, 0, "Test insert value");
  wrapper2.builder.insertText(stringNodeId, 0, "Insert at beginning ");

  wrapper1.applyOpsFrom(wrapper2);
  wrapper2.applyOpsFrom(wrapper1);

  wrapper1.builder.deleteText(stringNodeId, 10, 15);
  wrapper2.builder.deleteText(stringNodeId, 20, 5);

  wrapper1.applyOpsFrom(wrapper2);
  wrapper2.applyOpsFrom(wrapper1);

  ASSERT_EQ(wrapper1.getNodeBlockValue(stringNodeId), "Insert at insert value");
  ASSERT_EQ(wrapper2.getNodeBlockValue(stringNodeId), "Insert at insert value");
}

TEST(BlockValueTest, Utf16SupportWorks)
{
  CoreTestWrapper wrapper;

  std::string expected;
  std::string result;

  //NOTE: string offsets are calculated in number of UTF-16 code *units*
  //  but the string data itself is assumed to be UTF-8 encoded

  NodeId stringNodeId = wrapper.builder.createNode(PrimitiveNodeTypes::StringValue());

  std::string value0 = "Test insert \xF0\x9F\x98\x80 \xF0\x9F\x98\x80 value";
  wrapper.builder.insertText(stringNodeId, 0, value0);
  result = static_cast<const BlockValueNode<char> *>(wrapper.core->getExistingNode(stringNodeId))->value.toString();
  expected = value0;
  ASSERT_STREQ(expected.data(), result.data());

  wrapper.builder.deleteText(stringNodeId, 12, 3);
  result = static_cast<const BlockValueNode<char> *>(wrapper.core->getExistingNode(stringNodeId))->value.toString();
  expected.erase(12, 5);
  ASSERT_STREQ(expected.data(), result.data());

  wrapper.builder.deleteText(stringNodeId, 12, 2);
  result = static_cast<const BlockValueNode<char> *>(wrapper.core->getExistingNode(stringNodeId))->value.toString();
  expected.erase(12, 4);
  ASSERT_STREQ(expected.data(), result.data());

  std::string value3 = "\xCB\x9F\xCB\x9F\xCB\x9F ";
  wrapper.builder.insertText(stringNodeId, 5, value3);
  result = static_cast<const BlockValueNode<char> *>(wrapper.core->getExistingNode(stringNodeId))->value.toString();
  expected.insert(5, value3);
  ASSERT_STREQ(expected.data(), result.data());

  wrapper.builder.deleteText(stringNodeId, 6, 2);
  result = static_cast<const BlockValueNode<char> *>(wrapper.core->getExistingNode(stringNodeId))->value.toString();
  expected.erase(7, 4);
  ASSERT_STREQ(expected.data(), result.data());

  wrapper.builder.deleteText(stringNodeId, 5, 1);
  result = static_cast<const BlockValueNode<char> *>(wrapper.core->getExistingNode(stringNodeId))->value.toString();
  expected.erase(5, 2);
  ASSERT_STREQ(expected.data(), result.data());

  wrapper.builder.deleteText(stringNodeId, 0, expected.length());
  result = static_cast<const BlockValueNode<char> *>(wrapper.core->getExistingNode(stringNodeId))->value.toString();
  expected.erase(0, expected.length());
  ASSERT_STREQ(expected.data(), result.data());
}

TEST(BlockValueTest, InvalidInputIsHandled)
{
  CoreTestWrapper wrapper;

  std::string expected;
  std::string result;

  NodeId stringNodeId = wrapper.builder.createNode(PrimitiveNodeTypes::StringValue());

  std::string value0 = "Test insert value";
  wrapper.builder.insertText(stringNodeId, 0, value0);
  result = static_cast<const BlockValueNode<char> *>(wrapper.core->getExistingNode(stringNodeId))->value.toString();
  expected = value0;
  ASSERT_STREQ(expected.data(), result.data());

  //insert empty string
  wrapper.builder.insertText(stringNodeId, 0, "");
  result = static_cast<const BlockValueNode<char> *>(wrapper.core->getExistingNode(stringNodeId))->value.toString();
  expected = value0;
  ASSERT_STREQ(expected.data(), result.data());

  //insert after end of string
  wrapper.builder.insertText(stringNodeId, value0.length() + 1, "test");
  result = static_cast<const BlockValueNode<char> *>(wrapper.core->getExistingNode(stringNodeId))->value.toString();
  expected.insert(expected.length(), "test");
  ASSERT_STREQ(expected.data(), result.data());

  //delete 0 length
  wrapper.builder.deleteText(stringNodeId, 0, 0);
  result = static_cast<const BlockValueNode<char> *>(wrapper.core->getExistingNode(stringNodeId))->value.toString();
  ASSERT_STREQ(expected.data(), result.data());

  //delete after end of string
  wrapper.builder.deleteText(stringNodeId, result.length() + 1, 1);
  result = static_cast<const BlockValueNode<char> *>(wrapper.core->getExistingNode(stringNodeId))->value.toString();
  ASSERT_STREQ(expected.data(), result.data());
}

TEST(BlockValueTest, UndoWorks)
{
  CoreTestWrapper wrapper;

  NodeId stringNodeId = wrapper.builder.createNode(PrimitiveNodeTypes::StringValue());

  std::string start = std::string(200, 'A');
  std::string result;
  Timestamp ts1 = wrapper.group([&](OperationBuilder & builder) {
    builder.insertText(stringNodeId, 0, start);
  });
  Timestamp ts2 = wrapper.group([&](OperationBuilder & builder) {
    builder.insertText(stringNodeId, 100, std::string(50, 'B'));
  });

  undoOperation(wrapper, wrapper.log, ts2);

  result = static_cast<const BlockValueNode<char> *>(wrapper.core->getExistingNode(stringNodeId))->value.toString();
  ASSERT_EQ(start, result);

  undoOperation(wrapper, wrapper.log, ts1);

  result = static_cast<const BlockValueNode<char> *>(wrapper.core->getExistingNode(stringNodeId))->value.toString();
  ASSERT_EQ("", result);
}

TEST(BlockValueTest, RedoWorks)
{
  CoreTestWrapper wrapper;

  NodeId stringNodeId = wrapper.builder.createNode(PrimitiveNodeTypes::StringValue().toString());

  std::string start = std::string(200, 'A');
  std::string expect = start;
  std::string result;
  Timestamp ts1 = wrapper.group([&](OperationBuilder & builder) {
    builder.insertText(stringNodeId, 0, start);
  });

  std::string insert = std::string(50, 'B');
  expect.insert(100, insert);
  Timestamp ts2 = wrapper.group([&](OperationBuilder & builder) {
    builder.insertText(stringNodeId, 100, insert);
  });

  Timestamp ts3 = undoOperation(wrapper, wrapper.log, ts1);
  Timestamp ts4 = undoOperation(wrapper, wrapper.log, ts2);

  result = static_cast<const BlockValueNode<char> *>(wrapper.core->getExistingNode(stringNodeId))->value.toString();
  ASSERT_EQ("", result);

  undoOperation(wrapper, wrapper.log, ts3);
  undoOperation(wrapper, wrapper.log, ts4);

  result = static_cast<const BlockValueNode<char> *>(wrapper.core->getExistingNode(stringNodeId))->value.toString();
  ASSERT_EQ(expect, result);
}

TEST(BlockValueTest, UnapplyWorks)
{
  CoreTestWrapper wrapper;

  NodeId stringNodeId = wrapper.builder.createNode(PrimitiveNodeTypes::StringValue().toString());

  std::string start = std::string(200, 'A');
  std::string result;
  Timestamp ts1 = wrapper.group([&](OperationBuilder & builder) {
    builder.insertText(stringNodeId, 0, start);
  });
  Timestamp ts2 = wrapper.group([&](OperationBuilder & builder) {
    builder.insertText(stringNodeId, 100, std::string(50, 'B'));
  });

  unapplyOperation(wrapper, wrapper.log, ts2);

  result = static_cast<const BlockValueNode<char> *>(wrapper.core->getExistingNode(stringNodeId))->value.toString();
  ASSERT_EQ(start, result);

  unapplyOperation(wrapper, wrapper.log, ts1);

  result = static_cast<const BlockValueNode<char> *>(wrapper.core->getExistingNode(stringNodeId))->value.toString();
  ASSERT_EQ("", result);
}

TEST(BlockValueTest, InsertAndReverseUnapplyWorks)
{
  CoreTestWrapper wrapper;

  NodeId stringNodeId = wrapper.builder.createNode(PrimitiveNodeTypes::StringValue().toString());

  std::vector<std::pair<Timestamp, std::string>> state_stack;
  std::string result;
  Timestamp ts;

  std::string expected = std::string(250, 'A');
  ts = wrapper.group([&](OperationBuilder & builder) {
    builder.insertText(stringNodeId, 0, expected);
  });
  state_stack.push_back(std::make_pair(ts, expected));

  std::srand(0);
  const int numberOfOperations = 500;
  for (int i = 0; i < numberOfOperations; i++)
  {
    if (std::rand() & 1 || expected.size() == 0)
    {
      std::string insert(10 + std::rand() % 30, 'A' + (std::rand() % 26));
      size_t offset = std::rand() % (expected.size() + 1);
      expected.insert(offset, insert);
      ts = wrapper.group([&](OperationBuilder & builder) {
        builder.insertText(stringNodeId, offset, insert);
      });
    }
    else
    {
      size_t offset = std::rand() % expected.size();
      size_t len = std::rand() % (expected.size() - offset);
      if (len == 0) len = 1;

      expected.erase(offset, len);
      ts = wrapper.group([&](OperationBuilder & builder) {
        wrapper.builder.deleteText(stringNodeId, offset, len);
      });
    }

    state_stack.push_back(std::make_pair(ts, expected));

    result.clear();
    wrapper.core->getNodeBlockValue(result, stringNodeId);
    ASSERT_EQ(expected, result);
  }

  const int numberOfJumps = 100;
  auto logBeforeEnd = --wrapper.log.end();
  OperationFilter currentFilter;
  for (int i = 0; i < numberOfJumps; i++)
  {
    size_t nextTsIdx = std::rand() % state_stack.size();

    VectorTimestamp targetClock;
    targetClock.update(state_stack[nextTsIdx].first);
    OperationFilter newFilter;
    newFilter.setClockRange(VectorTimestamp(), targetClock);

    for (auto it = logBeforeEnd; ; --it)
    {
      auto op = *it;
      auto tsOp = reinterpret_cast<const LogOperation *>(op.data());
      bool hasOp = currentFilter.filter(*tsOp);
      bool needsOp = newFilter.filter(*tsOp);
      if (hasOp && !needsOp)
      {
        RefCounted<const LogOperation> rc(tsOp);
        wrapper.core->unapplyOperation(rc);
        rc.release();
      }

      if (it == wrapper.log.begin()) break;
    }
    for (auto it = wrapper.log.begin(); ; ++it)
    {
      auto op = *it;
      auto tsOp = reinterpret_cast<const LogOperation *>(op.data());
      bool hasOp = currentFilter.filter(*tsOp);
      bool needsOp = newFilter.filter(*tsOp);
      if (!hasOp && needsOp)
      {
        RefCounted<const LogOperation> rc(tsOp);
        wrapper.core->applyOperation(rc);
        rc.release();
      }

      if (it == logBeforeEnd) break;
    }

    // filterOperations(wrapper, wrapper.log, currentFilter, newFilter);
    wrapper.core->clock = targetClock;
    currentFilter = newFilter;

    result.clear();
    wrapper.core->getNodeBlockValue(result, stringNodeId);
    ASSERT_EQ(state_stack[nextTsIdx].second, result);
  }
}

TEST(BlockValueTest, InheritanceWorks)
{
  CoreTestWrapper wrapper;

  std::string expected0 = std::string(100, 'a');
  std::string expected1;
  std::string expected2;

  wrapper.types["type0"] = createTypeSpec([&](OperationBuilder & builder)
  {
    NodeId stringNodeId = builder.createNode(PrimitiveNodeTypes::StringValue());
    builder.insertText(stringNodeId, 0, expected0);
  }, wrapper.types);

  wrapper.types["type1"] = createTypeSpecFromRoot("type0",
  [&](const NodeId & rootNodeId, OperationBuilder & builder)
  {
    std::string insert = std::string(100, 'b');
    builder.insertText(rootNodeId, 0, insert);
    expected1 = expected0;
    expected1.insert(0, insert);
  }, wrapper.types);

  wrapper.types["type2"] = createTypeSpecFromRoot("type0",
  [&](const NodeId & rootNodeId, OperationBuilder & builder)
  {
    builder.deleteText(rootNodeId, 50, 25);
    expected2 = expected0;
    expected2.erase(50, 25);
  }, wrapper.types);

  NodeId resultNodeId;

  resultNodeId = wrapper.builder.createNode("type0");
  wrapper.resolveTypes();
  ASSERT_EQ(wrapper.getNodeBlockValue(resultNodeId), expected0);

  resultNodeId = wrapper.builder.createNode("type1");
  wrapper.resolveTypes();
  ASSERT_EQ(wrapper.getNodeBlockValue(resultNodeId), expected1);

  resultNodeId = wrapper.builder.createNode("type2");
  wrapper.resolveTypes();
  ASSERT_EQ(wrapper.getNodeBlockValue(resultNodeId), expected2);
}

TEST(BlockValueTest, EventsWork) {
    std::string result;
    auto eventHandler = [&](const CoreTestWrapper& wrapper, const Event& event) {
        auto insertEvent = dynamic_cast<const NodeBlockValueInsertedEvent*>(&event);
        if (insertEvent != nullptr) {
            result.insert(insertEvent->offset, std::string(insertEvent->str, insertEvent->length));
            ASSERT_EQ(result, wrapper.getNodeBlockValue(insertEvent->nodeId));
            return;
        }
        auto deleteEvent = dynamic_cast<const NodeBlockValueDeletedEvent*>(&event);
        if (deleteEvent != nullptr) {
            result.erase(deleteEvent->offset, deleteEvent->length);
            ASSERT_EQ(result, wrapper.getNodeBlockValue(deleteEvent->nodeId));
            return;
        }
    };

    CoreTestWrapper wrapper(eventHandler);
    NodeId stringNodeId = wrapper.builder.createNode(PrimitiveNodeTypes::StringValue().toString());
    std::vector<std::pair<Timestamp, std::string>> state_stack;
    Timestamp ts;
    std::string expected;

    expected = "Hello, World!";
    ts = wrapper.group([&](OperationBuilder& builder) {
        builder.insertText(stringNodeId, 0, expected);
    });
    state_stack.push_back(std::make_pair(ts, expected));
    ASSERT_EQ(expected, result);

    expected = "Hello, !";
    ts = wrapper.group([&](OperationBuilder& builder) {
        builder.deleteText(stringNodeId, 7, 5);
    });
    state_stack.push_back(std::make_pair(ts, expected));
    ASSERT_EQ(expected, result);

    for (int i = state_stack.size() - 1; i >= 0; i--) {
        auto& item = state_stack[i];
        unapplyOperation(wrapper, wrapper.log, item.first);
        if (i == 0) {
            break;
        }
        ASSERT_EQ(state_stack[i - 1].second, result);
    }

    wrapper.core->clock = VectorTimestamp();
    for (int i = 0; i < state_stack.size(); i++) {
        auto& item = state_stack[i];
        applyOperation(wrapper, wrapper.log, item.first);
        ASSERT_EQ(state_stack[i].second, result);
    }
}

TEST(BlockValueTest, EventsWorkWithUtf16Characters) {
    std::string expectedEventStr;
    size_t expectedEventOffset;
    size_t expectedEventLength;
    bool eventFired = false;

    auto eventHandler = [&](const CoreTestWrapper& wrapper, const Event& event) {
        auto insertEvent = dynamic_cast<const NodeBlockValueInsertedEvent*>(&event);
        if (insertEvent != nullptr) {
            ASSERT_FALSE(eventFired);
            eventFired = true;
            ASSERT_EQ(expectedEventStr, std::string(insertEvent->str, insertEvent->length));
            ASSERT_EQ(expectedEventOffset, insertEvent->offset);
            return;
        }
        auto deleteEvent = dynamic_cast<const NodeBlockValueDeletedEvent*>(&event);
        if (deleteEvent != nullptr) {
            ASSERT_FALSE(eventFired);
            eventFired = true;
            ASSERT_EQ(expectedEventOffset, deleteEvent->offset);
            ASSERT_EQ(expectedEventLength, deleteEvent->length);
            return;
        }
    };

    CoreTestWrapper wrapper(eventHandler);
    NodeId stringNodeId = wrapper.builder.createNode(PrimitiveNodeTypes::StringValue().toString());

    expectedEventStr = "Hello, 😀😀!";
    expectedEventOffset = 0;
    eventFired = false;
    wrapper.group([&](OperationBuilder& builder) {
        builder.insertText(stringNodeId, 0, "Hello, 😀😀!");
    });
    ASSERT_TRUE(eventFired);
    ASSERT_EQ("Hello, 😀😀!", wrapper.getNodeBlockValue(stringNodeId));

    expectedEventStr = "👍";
    expectedEventOffset = 11;
    eventFired = false;
    wrapper.group([&](OperationBuilder& builder) {
        builder.insertText(stringNodeId, 11, "👍");
    });
    ASSERT_TRUE(eventFired);
    ASSERT_EQ("Hello, 😀😀👍!", wrapper.getNodeBlockValue(stringNodeId));

    expectedEventOffset = 9;
    expectedEventLength = 2;
    eventFired = false;
    wrapper.group([&](OperationBuilder& builder) {
        builder.deleteText(stringNodeId, 9, 2);
    });
    ASSERT_TRUE(eventFired);
    ASSERT_EQ("Hello, 😀👍!", wrapper.getNodeBlockValue(stringNodeId));

    expectedEventOffset = 7;
    expectedEventLength = 2;
    eventFired = false;
    wrapper.group([&](OperationBuilder& builder) {
        builder.deleteText(stringNodeId, 7, 2);
    });
    ASSERT_TRUE(eventFired);
    ASSERT_EQ("Hello, 👍!", wrapper.getNodeBlockValue(stringNodeId));
}

TEST(BlockValueTest, DISABLED_OutOfOrderEventsWork)
{
  std::string result;

  auto eventHandler = [&](const CoreTestWrapper & wrapper, const Event & event)
  {
    auto insertEvent = dynamic_cast<const NodeBlockValueInsertedEvent *>(&event);
    if (insertEvent != nullptr)
    {
      result.insert(insertEvent->offset,
        std::string(insertEvent->str, insertEvent->length));

      ASSERT_EQ(result, wrapper.getNodeBlockValue(insertEvent->nodeId));
      return;
    }

    auto deleteEvent = dynamic_cast<const NodeBlockValueDeletedEvent *>(&event);
    if (deleteEvent != nullptr)
    {
      result.erase(deleteEvent->offset, deleteEvent->length);

      ASSERT_EQ(result, wrapper.getNodeBlockValue(deleteEvent->nodeId));
      return;
    }
  };

  CoreTestWrapper wrapper(eventHandler);

  NodeId stringNodeId = wrapper.builder.createNode(PrimitiveNodeTypes::StringValue().toString());

  std::vector<std::pair<Timestamp, std::string>> state_stack;
  Timestamp ts;

  std::string expected = std::string(250, 'A');
  ts = wrapper.group([&](OperationBuilder & builder) {
    builder.insertText(stringNodeId, 0, expected);
  });
  state_stack.push_back(std::make_pair(ts, expected));

  std::srand(0);
  for (int i = 0; i < 500; i++)
  {
    if (std::rand() & 1 || expected.size() == 0)
    {
      std::string insert(25, 'A' + (std::rand() % 26));
      size_t offset = std::rand() % (expected.size() + 1);
      expected.insert(offset, insert);
      ts = wrapper.group([&](OperationBuilder & builder) {
        builder.insertText(stringNodeId, offset, insert);
      });
    }
    else
    {
      size_t offset = std::rand() % expected.size();
      size_t len = std::rand() % (expected.size() - offset);
      if (len == 0) len = 1;

      expected.erase(offset, len);
      ts = wrapper.group([&](OperationBuilder & builder) {
        builder.deleteText(stringNodeId, offset, len);
      });
    }

    state_stack.push_back(std::make_pair(ts, expected));

    ASSERT_EQ(expected, result);
  }

  for (int i = state_stack.size() - 1; i >= 0; i--)
  {
    auto item = state_stack[i];
    unapplyOperation(wrapper, wrapper.log, item.first);

    if (i == 0)
    {
      break;
    }
    ASSERT_EQ(state_stack[i - 1].second, result);
  }

  wrapper.core->clock = VectorTimestamp();

  auto rng = std::default_random_engine {};
  std::shuffle(state_stack.begin(), state_stack.end(), rng);

  for (int i = 0; i < state_stack.size(); i++)
  {
    auto item = state_stack[i];
    applyOperation(wrapper, wrapper.log, item.first);
  }

  ASSERT_EQ(state_stack[state_stack.size() - 1].second, result);
}

TEST(BlockValueTest, DoubleApplyDoesNothing)
{
  CoreTestWrapper wrapper;

  auto nodeId = wrapper.builder.createNode(PrimitiveNodeTypes::StringValue());
  auto ts = wrapper.builder.getNextTimestamp();
  wrapper.builder.insertText(nodeId, 0, "AAAAA");

  ASSERT_EQ(wrapper.getNodeBlockValue(nodeId), "AAAAA");

  applyOperation(wrapper, wrapper.log, ts);
  ASSERT_EQ(wrapper.getNodeBlockValue(nodeId), "AAAAA");

  unapplyOperation(wrapper, wrapper.log, ts);
  ASSERT_EQ(wrapper.getNodeBlockValue(nodeId), "");

  applyOperation(wrapper, wrapper.log, ts);
  ASSERT_EQ(wrapper.getNodeBlockValue(nodeId), "AAAAA");
}

TEST(BlockValueTest, DoubleUnapplyDoesNothing)
{
  CoreTestWrapper wrapper;

  auto nodeId = wrapper.builder.createNode(PrimitiveNodeTypes::StringValue());
  auto ts = wrapper.builder.getNextTimestamp();
  wrapper.builder.insertText(nodeId, 0, "AAAAA");

  ASSERT_EQ(wrapper.getNodeBlockValue(nodeId), "AAAAA");

  unapplyOperation(wrapper, wrapper.log, ts);
  ASSERT_EQ(wrapper.getNodeBlockValue(nodeId), "");

  unapplyOperation(wrapper, wrapper.log, ts);
  ASSERT_EQ(wrapper.getNodeBlockValue(nodeId), "");

  applyOperation(wrapper, wrapper.log, ts);
  ASSERT_EQ(wrapper.getNodeBlockValue(nodeId), "AAAAA");
}

TEST(BlockValueTest, UnapplyAndApplyPreservesDeletions)
{
  CoreTestWrapper wrapper;

  wrapper.builder.setSiteId(1);
  auto nodeId = wrapper.builder.createNode(PrimitiveNodeTypes::StringValue());
  wrapper.builder.insertText(nodeId, 0, "AAAAA");
  wrapper.builder.insertText(nodeId, 5, "BBBBB");
  wrapper.builder.insertText(nodeId, 10, "CCCCC");

  wrapper.builder.setSiteId(2);
  wrapper.builder.deleteText(nodeId, 0, 10);

  OperationFilter filter;
  filter.setSiteFilter(1);

  ASSERT_EQ(wrapper.getNodeBlockValue(nodeId), "CCCCC");

  unapplyOperations(wrapper, wrapper.log, filter);

  ASSERT_EQ(wrapper.getNodeBlockValue(nodeId), "");

  applyOperations(wrapper, wrapper.log, filter);

  ASSERT_EQ(wrapper.getNodeBlockValue(nodeId), "CCCCC");
}

TEST(BlockValueTest, UnapplyAndApplyPreservesOrder)
{
  CoreTestWrapper wrapper;

  auto nodeId = wrapper.builder.createNode(PrimitiveNodeTypes::StringValue());

  wrapper.builder.insertText(nodeId, 0, "AAAAA");
  auto ts = wrapper.builder.getNextTimestamp();
  wrapper.builder.insertText(nodeId, 5, "BBBBB");
  wrapper.builder.insertText(nodeId, 10, "CCCCC");

  ASSERT_EQ(wrapper.getNodeBlockValue(nodeId), "AAAAABBBBBCCCCC");

  unapplyOperation(wrapper, wrapper.log, ts);

  ASSERT_EQ(wrapper.getNodeBlockValue(nodeId), "AAAAACCCCC");

  applyOperation(wrapper, wrapper.log, ts);

  ASSERT_EQ(wrapper.getNodeBlockValue(nodeId), "AAAAABBBBBCCCCC");
}

TEST(BlockValueTest, GeneralFuzz)
{
  std::string result;

  auto eventHandler = [&](const CoreTestWrapper & wrapper, const Event & event)
  {
    auto insertEvent = dynamic_cast<const NodeBlockValueInsertedEvent *>(&event);
    if (insertEvent != nullptr)
    {
      result.insert(insertEvent->offset,
        std::string(insertEvent->str, insertEvent->length));

      //at the time an event is raised, the node's value should be consistent
      //  with the computed value from the events
      ASSERT_EQ(result, wrapper.getNodeBlockValue(insertEvent->nodeId));
      return;
    }

    auto deleteEvent = dynamic_cast<const NodeBlockValueDeletedEvent *>(&event);
    if (deleteEvent != nullptr)
    {
      result.erase(deleteEvent->offset, deleteEvent->length);

      ASSERT_EQ(result, wrapper.getNodeBlockValue(deleteEvent->nodeId));
      return;
    }
  };

  CoreTestWrapper wrapper(eventHandler);

  NodeId stringNodeId = wrapper.builder.createNode(PrimitiveNodeTypes::StringValue().toString());

  std::vector<std::pair<Timestamp, std::string>> state_stack;
  Timestamp ts;

  std::string expected = std::string(250, 'A');
  ts = wrapper.group([&](OperationBuilder & builder) {
    builder.insertText(stringNodeId, 0, expected);
  });
  state_stack.push_back(std::make_pair(ts, expected));

  std::srand(100);
  const int numberOfOperations = 1000;
  for (int i = 0; i < numberOfOperations; i++)
  {
    if (std::rand() & 1 || expected.size() == 0)
    {
      std::string insert(10 + std::rand() % 30, 'A' + (std::rand() % 26));
      size_t offset = std::rand() % (expected.size() + 1);
      expected.insert(offset, insert);
      ts = wrapper.group([&](OperationBuilder & builder) {
        builder.insertText(stringNodeId, offset, insert);
      });
    }
    else
    {
      size_t offset = std::rand() % expected.size();
      size_t len = std::rand() % (expected.size() - offset);
      if (len == 0) len = 1;

      expected.erase(offset, len);
      ts = wrapper.group([&](OperationBuilder & builder) {
        builder.deleteText(stringNodeId, offset, len);
      });
    }

    state_stack.push_back(std::make_pair(ts, expected));

    ASSERT_EQ(expected, result);
  }

  for (int i = state_stack.size() - 1; i >= 0; i--)
  {
    auto & item = state_stack[i];
    unapplyOperation(wrapper, wrapper.log, item.first);

    if (i == 0)
    {
      break;
    }
    ASSERT_EQ(state_stack[i - 1].second, result);
  }

  wrapper.core->clock = VectorTimestamp();

  for (int i = 0; i < state_stack.size(); i++)
  {
    auto & item = state_stack[i];
    applyOperation(wrapper, wrapper.log, item.first);

    ASSERT_EQ(state_stack[i].second, result);
  }
}