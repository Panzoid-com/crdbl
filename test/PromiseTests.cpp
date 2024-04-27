#include <gtest/gtest.h>
#include <Promise.h>

TEST(PromiseTest, StaticPromisesWork)
{
  auto promise = Promise<void>::Resolve();
  EXPECT_EQ(promise.isSettled(), true);
  EXPECT_EQ(promise.isResolved(), true);

  auto promise2 = Promise<void>::Reject();
  EXPECT_EQ(promise2.isSettled(), true);
  EXPECT_EQ(promise2.isResolved(), false);

  auto promise3 = Promise<int>::Resolve(5);
  EXPECT_EQ(promise3.isSettled(), true);
  EXPECT_EQ(promise3.isResolved(), true);
}

TEST(PromiseTest, ResolvingStaticPromiseDoesNothing)
{
  auto promise = Promise<void>::Reject();
  promise.resolve();
  EXPECT_EQ(promise.isSettled(), true);
  EXPECT_EQ(promise.isResolved(), false);
}

TEST(PromiseTest, RejectingStaticPromiseDoesNothing)
{
  auto promise = Promise<void>::Resolve();
  promise.reject();
  EXPECT_EQ(promise.isSettled(), true);
  EXPECT_EQ(promise.isResolved(), true);
}

TEST(PromiseTest, StaticResolvedPromiseCallsThenCallbackImmediately)
{
  auto promise = Promise<void>::Resolve();
  int calledCount = 0;
  promise.then([&calledCount]()
  {
    calledCount++;
  });
  EXPECT_EQ(calledCount, 1);
}

TEST(PromiseTest, DynamicResolvedPromiseCallsThenCallbackImmediately)
{
  auto promise = Promise<void>();
  int calledCount = 0;
  promise.then([&calledCount]()
  {
    calledCount++;
  });
  EXPECT_EQ(calledCount, 0);
  promise.resolve();
  EXPECT_EQ(calledCount, 1);

  promise.then([&calledCount]()
  {
    calledCount++;
  });
  EXPECT_EQ(calledCount, 2);
}

TEST(PromiseTest, StaticRejectedPromiseDoesNotCallThenCallback)
{
  auto promise = Promise<void>::Reject();
  int calledCount = 0;
  promise.then([&calledCount]()
  {
    calledCount++;
  });
  EXPECT_EQ(calledCount, 0);
}

TEST(PromiseTest, PromiseThenWorks)
{
  auto promise = Promise<void>();
  int calledCount = 0;
  promise.then([&calledCount]()
  {
    calledCount++;
  });
  promise.resolve();
  EXPECT_EQ(calledCount, 1);
  promise.resolve();
  EXPECT_EQ(calledCount, 1);
}

TEST(PromiseTest, PromiseThenChainingWorks)
{
  auto promise = Promise<void>();
  int calledCount = 0;
  promise.then([&calledCount]()
  {
    calledCount++;
  }).then([&calledCount]()
  {
    calledCount++;
  });
  promise.resolve();
  EXPECT_EQ(calledCount, 2);
}

TEST(PromiseTest, PromiseThenChainingWorksWithReturnValue)
{
  auto promise = Promise<int>();
  int calledCount = 0;
  int sum = 0;
  promise.then([&calledCount, &sum](int value)
  {
    calledCount++;
    sum += value;
    return value + 1;
  }).then([&calledCount, &sum](int value)
  {
    calledCount++;
    sum += value;
  });
  promise.resolve(5);
  EXPECT_EQ(calledCount, 2);
  EXPECT_EQ(sum, 11);
}

TEST(PromiseTest, PromiseThenChainingWorksWithInnerPromise)
{
  auto promise = Promise<int>();
  auto promise2 = Promise<int>();
  int calledCount = 0;
  int sum = 0;
  promise.then([&calledCount, &sum, &promise2](int value)
  {
    calledCount++;
    sum += value;
    return promise2;
  }).then([&calledCount, &sum](int value)
  {
    calledCount++;
    sum += value;
  });
  promise.resolve(5);
  promise2.resolve(6);
  EXPECT_EQ(calledCount, 2);
  EXPECT_EQ(sum, 11);

}

TEST(PromiseTest, PromiseFinallyWorksWithResolve)
{
  auto promise = Promise<void>();
  int calledCount = 0;
  promise
  .then([&calledCount]()
  {
    EXPECT_EQ(calledCount, 0);
    calledCount++;
  })
  .finally([&calledCount]()
  {
    EXPECT_EQ(calledCount, 1);
    calledCount++;
  });
  promise.resolve();
  EXPECT_EQ(calledCount, 2);
}

TEST(PromiseTest, PromiseFinallyWorksWithReject)
{
  auto promise = Promise<void>();
  int calledCount = 0;
  promise
  .then([]()
  {
    throw std::runtime_error("Error");
  })
  .finally([&calledCount]()
  {
    calledCount++;
  });
  promise.reject();
  EXPECT_EQ(calledCount, 1);

  promise.reject();
  EXPECT_EQ(calledCount, 1);
}