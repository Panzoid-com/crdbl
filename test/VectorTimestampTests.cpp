#include <gtest/gtest.h>
#include <algorithm>
#include <Core.h>
#include "helpers.h"

TEST(VectorTimestampTest, ComparisonWorks)
{
  VectorTimestamp ts0, ts1;

  ts0 = VectorTimestamp(std::vector<uint32_t>{});
  ts1 = VectorTimestamp(std::vector<uint32_t>{});
  ASSERT_EQ(ts0, ts1);
  ASSERT_FALSE(ts0 < ts1);
  ASSERT_FALSE(ts1 < ts0);
  ASSERT_TRUE(ts0.isEmpty());

  ts0 = VectorTimestamp(std::vector<uint32_t>{ 1, 2, 3, 4 });
  ts1 = VectorTimestamp(std::vector<uint32_t>{ 1, 2, 3, 4 });
  ASSERT_EQ(ts0, ts1);
  ASSERT_FALSE(ts0 < ts1);
  ASSERT_FALSE(ts1 < ts0);
  ASSERT_FALSE(ts0.isEmpty());

  ts0 = VectorTimestamp(std::vector<uint32_t>{ 1, 2, 2, 4 });
  ts1 = VectorTimestamp(std::vector<uint32_t>{ 1, 2, 3, 4 });
  ASSERT_NE(ts0, ts1);
  ASSERT_LT(ts0, ts1);
  ASSERT_FALSE(ts1 < ts0);

  ts0 = VectorTimestamp(std::vector<uint32_t>{ 1, 2, 2, 4, 5 });
  ts1 = VectorTimestamp(std::vector<uint32_t>{ 1, 2, 3, 4 });
  ASSERT_NE(ts0, ts1);
  ASSERT_LT(ts0, ts1);
  ASSERT_LT(ts1, ts0);

  ts0 = VectorTimestamp(std::vector<uint32_t>{ 1, 2, 3, 4 });
  ts1 = VectorTimestamp(std::vector<uint32_t>{ 1, 2, 3, 4, 5 });
  ASSERT_NE(ts0, ts1);
  ASSERT_LT(ts0, ts1);
  ASSERT_FALSE(ts1 < ts0);

  ts0 = VectorTimestamp(std::vector<uint32_t>{ 1, 2, 3, 4, 0 });
  ts1 = VectorTimestamp(std::vector<uint32_t>{ 1, 2, 3, 4 });
  ASSERT_EQ(ts0, ts1);
  ASSERT_FALSE(ts0 < ts1);
  ASSERT_FALSE(ts1 < ts0);
  ASSERT_FALSE(ts0.isEmpty());

  ts0 = VectorTimestamp();
  ts1 = VectorTimestamp(std::vector<uint32_t>{ 0 });
  ASSERT_EQ(ts0, ts1);
  ASSERT_FALSE(ts0 < ts1);
  ASSERT_FALSE(ts1 < ts0);
  ASSERT_TRUE(ts0.isEmpty());
  ASSERT_TRUE(ts1.isEmpty());
}

TEST(VectorTimestampTest, UpdateWorks)
{
  VectorTimestamp ts;

  ts = VectorTimestamp(std::vector<uint32_t>{});

  ts.update(Timestamp(10, 1));
  ASSERT_EQ(ts, VectorTimestamp(std::vector<uint32_t>{ 0, 10 }));

  ts.update(Timestamp(10, 3));
  ASSERT_EQ(ts, VectorTimestamp(std::vector<uint32_t>{ 0, 10, 0, 10 }));

  ts.update(Timestamp(5, 1));
  ts.update(Timestamp(10, 1));
  ASSERT_EQ(ts, VectorTimestamp(std::vector<uint32_t>{ 0, 10, 0, 10 }));

  ts.update(Timestamp(11, 1));
  ASSERT_EQ(ts, VectorTimestamp(std::vector<uint32_t>{ 0, 11, 0, 10 }));

  ts.update(Timestamp(0, 4));
  ASSERT_EQ(ts, VectorTimestamp(std::vector<uint32_t>{ 0, 11, 0, 10 }));
}

TEST(VectorTimestampTest, MaxWorks)
{
  VectorTimestamp ts;

  ts = VectorTimestamp(std::vector<uint32_t>{});
  ASSERT_EQ(ts.getMaxClock(), 0);

  ts.update(Timestamp(10, 1));
  ts.update(Timestamp(4, 3));
  ts.update(Timestamp(12, 4));
  ASSERT_EQ(ts.getMaxClock(), 12);

  ts.update(Timestamp(0, 5));
  ASSERT_EQ(ts.getMaxClock(), 12);

  ts = VectorTimestamp(std::vector<uint32_t>{ 0, 11, 0, 10, 0 });
  ASSERT_EQ(ts.getMaxClock(), 11);
}

TEST(VectorTimestampTest, MergeWorks)
{
  VectorTimestamp ts1(std::vector<uint32_t>{ 0, 11, 0, 10, 0 });
  VectorTimestamp ts2(std::vector<uint32_t>{ 0, 100, 0, 0, 0, 10, 0, 20 });
  VectorTimestamp ts3 = ts2;
  VectorTimestamp ts4;

  auto expected = std::vector<uint32_t>{ 0, 100, 0, 10, 0, 10, 0, 20 };

  ts3.merge(ts1);
  ASSERT_EQ(ts3.getVector(), expected);

  ts1.merge(ts2);
  ASSERT_EQ(ts1.getVector(), expected);
  ASSERT_EQ(ts1.getMaxClock(), 100);

  ts4.merge(ts1);
  ts4.merge(ts2);
  ASSERT_EQ(ts4.getVector(), expected);
}