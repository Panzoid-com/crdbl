#pragma once
#include "Promise.h"

class PromiseAll
{
public:
  void add(Promise<void> promise)
  {
    if (promise.isSettled())
    {
      return;
    }

    promiseCount++;

    // This is a little sketchy
    // Right now, PromiseAll is only ever used in a way that guarantees that
    //   it will outlive the promises it is managing
    // However, there is nothing guaranteeing that
    promise.then([this]()
    {
      ++resolvedCount;
    });
    promise.finally([this]()
    {
      ++settledCount;
      if (settledCount == promiseCount)
      {
        if (resolvedCount != promiseCount)
        {
          allResolvedPromise.reject();
          return;
        }

        allResolvedPromise.resolve();
      }
    });
  }

  Promise<void> getPromise()
  {
    if (promiseCount == 0)
    {
      return Promise<void>::Resolve();
    }

    return allResolvedPromise;
  }

private:
  Promise<void> allResolvedPromise;
  size_t promiseCount = 0;
  size_t resolvedCount = 0;
  size_t settledCount = 0;
};