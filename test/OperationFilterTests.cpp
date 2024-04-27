#include <gtest/gtest.h>
#include <Core.h>
#include <LogOperation.h>
#include "helpers.h"

TEST(OperationFilterTest, OperationFilterWorks)
{
  OperationFilter opFilter;

  char data[sizeof(LogOperation) + sizeof(NoOpOperation)];
  LogOperation * tsOp = reinterpret_cast<LogOperation *>(&data);
  Operation * op = reinterpret_cast<Operation *>(&tsOp->op);
  op->type = OperationType::NoOpOperation;

  tsOp->tag = Tag::Default();

  Tag testTag = Tag{};
  testTag.value[0] = 1;

  ASSERT_TRUE(testTag != Tag::Default());

  //AllowsAnyTimestampByDefault
  tsOp->ts = { 1, 1 };
  ASSERT_TRUE(opFilter.filter(*tsOp));
  tsOp->ts = { 100, 2 };
  ASSERT_TRUE(opFilter.filter(*tsOp));

  //ClockRangeWorks
  opFilter.reset();
  opFilter.setClockRange(VectorTimestamp(std::vector<uint32_t>{ 0, 10, 100 }),
    VectorTimestamp());
  ASSERT_FALSE(opFilter.filter(*tsOp));
  opFilter.setClockRange(VectorTimestamp(std::vector<uint32_t>{ 0, 10, 99 }),
    VectorTimestamp());
  ASSERT_TRUE(opFilter.filter(*tsOp));
  opFilter.setClockRange(
    VectorTimestamp(std::vector<uint32_t>{ 0, 10, 98 }),
    VectorTimestamp(std::vector<uint32_t>{ 0, 10, 100 }));
  ASSERT_TRUE(opFilter.filter(*tsOp));
  opFilter.setClockRange(
    VectorTimestamp(std::vector<uint32_t>{ 0, 10, 98 }),
    VectorTimestamp(std::vector<uint32_t>{ 0, 10, 99 }));
  ASSERT_FALSE(opFilter.filter(*tsOp));

  //TagRangeAllowsAllTagsByDefault
  opFilter.reset();
  ASSERT_TRUE(opFilter.filter(*tsOp));
  tsOp->tag = testTag;
  ASSERT_TRUE(opFilter.filter(*tsOp));

  //TagRangeRequiresSpecifiedTag
  opFilter.setTagClockRange(Tag::Default(), VectorTimestamp(), VectorTimestamp());
  ASSERT_FALSE(opFilter.filter(*tsOp));
  tsOp->tag = Tag::Default();
  ASSERT_TRUE(opFilter.filter(*tsOp));

  //TagRangeClockFilterWorks
  tsOp->tag = Tag::Default();
  opFilter.setTagClockRange(Tag::Default(),
    VectorTimestamp(std::vector<uint32_t>{ 0, 10, 98 }),
    VectorTimestamp(std::vector<uint32_t>{ 0, 10, 100 }));
  ASSERT_TRUE(opFilter.filter(*tsOp));
  tsOp->tag = testTag;
  ASSERT_FALSE(opFilter.filter(*tsOp));
  tsOp->tag = Tag::Default();
  tsOp->ts = { 101, 2 };
  ASSERT_FALSE(opFilter.filter(*tsOp));

  //InvertWorks
  opFilter.reset().invert();
  ASSERT_FALSE(opFilter.filter(*tsOp));
  opFilter.reset()
    .setTagClockRange(Tag::Default(),
      VectorTimestamp(std::vector<uint32_t>{ 0, 10, 99 }),
      VectorTimestamp())
    .setSiteFilter(2);
  ASSERT_TRUE(opFilter.filter(*tsOp));
  opFilter.invert();
  ASSERT_FALSE(opFilter.filter(*tsOp));
}

TEST(OperationFilterTest, SiteFilterWorks)
{
  OperationFilter opFilter;

  char data[sizeof(LogOperation) + sizeof(NoOpOperation)];
  LogOperation * tsOp = reinterpret_cast<LogOperation *>(&data);
  Operation * op = reinterpret_cast<Operation *>(&tsOp->op);
  op->type = OperationType::NoOpOperation;

  tsOp->tag = Tag::Default();

  Tag testTag = Tag{};
  testTag.value[0] = 1;

  ASSERT_TRUE(testTag != Tag::Default());

  tsOp->ts = { 1, 2 };
  ASSERT_TRUE(opFilter.filter(*tsOp));

  opFilter.setSiteFilter(1);
  ASSERT_FALSE(opFilter.filter(*tsOp));
  opFilter.setSiteFilter(2);
  ASSERT_TRUE(opFilter.filter(*tsOp));

  opFilter.setSiteFilterInvert(true);
  ASSERT_FALSE(opFilter.filter(*tsOp));

  opFilter.reset().setSiteFilter(1);
  ASSERT_FALSE(opFilter.filter(*tsOp));
}

TEST(OperationFilterTest, FilterBoundsSerializationAndDeserializationWorks)
{
  OperationFilter opFilter;

  opFilter.setClockRange(VectorTimestamp(),
    VectorTimestamp(std::vector<uint32_t>{ 0, 10, 100 }));

  opFilter.setTagClockRange(Tag::Default(), VectorTimestamp(),
    VectorTimestamp(std::vector<uint32_t>{ 0, 11, 99 }));

  opFilter.setTagClockRange(Tag{1, 1, 1, 1}, VectorTimestamp(),
    VectorTimestamp(std::vector<uint32_t>{ 0, 13, 101 }));

  auto data = OperationFilter::Serialize("standard_filter_v1_bounds", opFilter);
  auto newOpFilter = OperationFilter::Deserialize("standard_filter_v1_bounds", data);

  ASSERT_EQ(opFilter, newOpFilter);
}

TEST(OperationFilterTest, FilterFullSerializationAndDeserializationWorks)
{
  OperationFilter opFilter;

  opFilter.setClockRange(
    VectorTimestamp(std::vector<uint32_t>{ 0, 9, 99 }),
    VectorTimestamp(std::vector<uint32_t>{ 0, 10, 100 }));

  opFilter.setTagClockRange(Tag::Default(),
    VectorTimestamp(std::vector<uint32_t>{ 0, 8, 98 }),
    VectorTimestamp(std::vector<uint32_t>{ 0, 11, 101 }));

  opFilter.setTagClockRange(Tag{1, 1, 1, 1},
    VectorTimestamp(std::vector<uint32_t>{ 0, 7, 97 }),
    VectorTimestamp(std::vector<uint32_t>{ 0, 12, 102 }));

  opFilter.setSiteFilter(2);
  opFilter.setSiteFilter(4);
  opFilter.setSiteFilterInvert(true);

  opFilter.invert();

  auto data = OperationFilter::Serialize("standard_filter_v1_full", opFilter);
  auto newOpFilter = OperationFilter::Deserialize("standard_filter_v1_full", data);

  ASSERT_EQ(opFilter, newOpFilter);
}