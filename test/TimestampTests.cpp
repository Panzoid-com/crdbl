#include <gtest/gtest.h>
#include <algorithm>
#include <Core.h>
#include "helpers.h"

TEST(TimestampTest, EqualityWorks)
{
  Timestamp ts0, ts1;

  ts0 = { 0, 0 };
  ts1 = { 0, 0 };
  ASSERT_EQ(ts0, ts1);

  ts0 = { 6, 3 };
  ts1 = { 6, 3 };
  ASSERT_EQ(ts0, ts1);

  ts0 = { 5, 3 };
  ts1 = { 6, 3 };
  ASSERT_NE(ts0, ts1);

  ts0 = { 6, 2 };
  ts1 = { 6, 3 };
  ASSERT_NE(ts0, ts1);

  ts0 = { 6, 2 };
  ts1 = { 5, 3 };
  ASSERT_NE(ts0, ts1);
}

TEST(TimestampTest, ComparisonWorks)
{
  Timestamp ts0, ts1;

  ts0 = { 0, 0 };
  ts1 = { 0, 0 };
  ASSERT_EQ(ts0, ts1);
  ASSERT_FALSE(ts0 < ts1);
  ASSERT_FALSE(ts1 < ts0);

  ts0 = { 6, 3 };
  ts1 = { 6, 3 };
  ASSERT_EQ(ts0, ts1);
  ASSERT_FALSE(ts0 < ts1);
  ASSERT_FALSE(ts1 < ts0);

  ts0 = { 5, 3 };
  ts1 = { 6, 3 };
  ASSERT_NE(ts0, ts1);
  ASSERT_LT(ts0, ts1);

  ts0 = { 6, 2 };
  ts1 = { 6, 3 };
  ASSERT_NE(ts0, ts1);
  ASSERT_LT(ts0, ts1);

  ts0 = { 6, 2 };
  ts1 = { 5, 3 };
  ASSERT_NE(ts0, ts1);
  ASSERT_LT(ts1, ts0);
}

TEST(TimestampTest, IncrementWorks)
{
  Timestamp ts = { 0, 0 };

  ++ts;
  ASSERT_EQ(ts, Timestamp(1, 0));

  ++ts;
  ASSERT_EQ(ts, Timestamp(2, 0));
}

TEST(TimestampTest, AddIntWorks)
{
  Timestamp ts = { 1, 0 };

  ASSERT_EQ(ts + 1, Timestamp(2, 0));
  ASSERT_EQ(ts + 3, Timestamp(4, 0));
}

TEST(TimestampTest, AddTimestampWorks)
{
  Timestamp ts0 = { 0, 0 };
  Timestamp ts1 = { 0, 0 };

  ts0 += ts1;
  ASSERT_EQ(ts0, Timestamp(0, 0));

  ts0 = { 1, 2 };
  ts1 = { 3, 4 };
  ts0 += ts1;
  ASSERT_EQ(ts0, Timestamp(4, 6));
}

TEST(TimestampTest, UpdateWorks)
{
  Timestamp ts = { 0, 0 };

  ts.update({ 0, 0 });
  ASSERT_EQ(ts, Timestamp(0, 0));

  ts.update({ 1, 0 });
  ASSERT_EQ(ts, Timestamp(1, 0));

  ts.update({ 0, 0 });
  ASSERT_EQ(ts, Timestamp(1, 0));

  ts.update({ 1, 1 });
  ASSERT_EQ(ts, Timestamp(1, 0));

  ts.update({ 2, 0 });
  ASSERT_EQ(ts, Timestamp(2, 0));
}