#pragma once
#include <vector>
#include <functional>
#include <type_traits>
#include <optional>
#include <variant>
#include <memory>

#include <iostream>

template <typename T>
class Promise;

template <typename T>
struct PromiseResultType
{
  using type = T;
};

template <typename T>
struct PromiseResultType<Promise<T>>
{
  using type = T;
};

template <typename T>
class Promise
{
public:
  using CallbackInternal = std::function<void(T)>;
  using FinallyCallbackInternal = std::function<void()>;

  Promise(std::optional<T> value) : content(value) {}
  Promise() : content(std::make_shared<PromiseInternal>()) {}

  static Promise<T> Resolve(T value)
  {
    return Promise<T>(value);
  }

  static Promise<T> Reject()
  {
    return Promise<T>(std::nullopt);
  }

  template <typename Callback>
  auto then(Callback callback) -> Promise<typename PromiseResultType<typename std::invoke_result<Callback, T>::type>::type>
  {
    using ReturnType = typename std::invoke_result<Callback, T>::type;
    using ResultType = typename PromiseResultType<ReturnType>::type;

    if (isSettled())
    {
      auto result = getResult();
      if (result.has_value())
      {
        if constexpr (std::is_same<ReturnType, Promise<ResultType>>::value)
        {
          return callback(result.value());
        }
        else if constexpr (std::is_same<ResultType, void>::value)
        {
          callback(result.value());
          return Promise<ResultType>::Resolve();
        }
        else
        {
          return Promise<ResultType>::Resolve(callback(result.value()));
        }
      }
      else
      {
        return Promise<ResultType>::Reject();
      }
    }

    auto internal = std::get<std::shared_ptr<PromiseInternal>>(content);
    Promise<ResultType> promise;

    //two cases: the return type is a Promise or not
    // if it's a Promise, then we need to call the resolve callback with the
    //   result of the inner promise
    // otherwise, we just call the resolve callback with the value
    if constexpr (std::is_same<ReturnType, Promise<ResultType>>::value)
    {
      auto callbackWrapper = [callback, promise](T value)
      {
        auto innerPromise = callback(value);
        if constexpr (std::is_same<ResultType, void>::value)
        {
          innerPromise.then([promise]()
          {
            promise.resolve();
          });
        }
        else
        {
          innerPromise.then([promise](ResultType innerValue)
          {
            promise.resolve(innerValue);
          });
        }
      };

      internal->resolveCallbacks.push_back(callbackWrapper);
    }
    else
    {
      auto callbackWrapper = [callback, promise](T value)
      {
        if constexpr (std::is_same<ResultType, void>::value)
        {
          callback(value);
          promise.resolve();
        }
        else
        {
          promise.resolve(callback(value));
        }
      };

      internal->resolveCallbacks.push_back(callbackWrapper);
    }

    //this is just a way to implement catch() without separate catch callbacks
    internal->finallyCallbacks.push_back([internal, promise]()
    {
      if (!internal->resolvedValue.has_value())
      {
        promise.reject();
      }
    });

    return promise;
  }

  template <typename Callback>
  auto finally(Callback callback) -> Promise<typename PromiseResultType<typename std::invoke_result<Callback>::type>::type>
  {
    using ReturnType = typename std::invoke_result<Callback>::type;
    using ResultType = typename PromiseResultType<ReturnType>::type;

    static_assert(std::is_same<ResultType, void>::value, "finally callback must return void");

    if (isSettled())
    {
      auto result = getResult();
      if (result.has_value())
      {
        callback();
        return Promise<ResultType>::Resolve();
      }
      else
      {
        callback();
        return Promise<ResultType>::Reject();
      }
    }

    auto internal = std::get<std::shared_ptr<PromiseInternal>>(content);
    Promise<ResultType> promise;

    if constexpr (std::is_same<ReturnType, Promise<ResultType>>::value)
    {
      auto callbackWrapper = [callback, promise]()
      {
        auto innerPromise = callback();
        innerPromise.finally([promise]()
        {
          if (innerPromise.isResolved())
          {
            promise.resolve();
          }
          else
          {
            promise.reject();
          }
        });
      };

      internal->finallyCallbacks.push_back(callbackWrapper);
    }
    else
    {
      auto callbackWrapper = [callback, promise]()
      {
        callback();
        promise.resolve();
      };

      internal->finallyCallbacks.push_back(callbackWrapper);
    }

    return promise;
  }

  void resolve(T value) const
  {
    if (!std::holds_alternative<std::shared_ptr<PromiseInternal>>(content))
    {
      return;
    }

    auto ptr = std::get<std::shared_ptr<PromiseInternal>>(content);
    if (ptr->settled)
    {
      return;
    }

    ptr->resolvedValue = value;
    ptr->settled = true;

    for (const auto & it : ptr->resolveCallbacks)
    {
      it(value);
    }

    doFinally(ptr);
  }

  void reject() const
  {
    if (!std::holds_alternative<std::shared_ptr<PromiseInternal>>(content))
    {
      return;
    }

    auto ptr = std::get<std::shared_ptr<PromiseInternal>>(content);
    if (ptr->settled)
    {
      return;
    }

    ptr->settled = true;

    doFinally(ptr);
  }

  bool isResolved() const
  {
    if (auto val = std::get_if<std::optional<T>>(&content))
    {
      return val->has_value();
    }

    return std::get<std::shared_ptr<PromiseInternal>>(content)->resolvedValue.has_value();
  }

  bool isSettled() const
  {
    if (std::holds_alternative<std::optional<T>>(content))
    {
      return true;
    }

    return std::get<std::shared_ptr<PromiseInternal>>(content)->settled;
  }

private:
  struct PromiseInternal
  {
    bool settled = false;
    std::optional<T> resolvedValue;
    std::vector<CallbackInternal> resolveCallbacks;
    std::vector<FinallyCallbackInternal> finallyCallbacks;
  };

  void doFinally(const std::shared_ptr<PromiseInternal> & internal) const
  {
    for (const auto & it : internal->finallyCallbacks)
    {
      it();
    }

    //this is important to free e.g. captured references in the callbacks
    internal->resolveCallbacks.clear();
    internal->finallyCallbacks.clear();
  }

  std::optional<T> getResult() const
  {
    if (auto val = std::get_if<std::optional<T>>(&content))
    {
      return *val;
    }

    return std::get<std::shared_ptr<PromiseInternal>>(content)->resolvedValue;
  }

  std::variant<std::optional<T>, std::shared_ptr<PromiseInternal>> content;
};


template <>
class Promise<void>
{
public:
  using CallbackInternal = std::function<void()>;
  using FinallyCallbackInternal = std::function<void()>;

  Promise(bool value) : content(value) {}
  Promise() : content(std::make_shared<PromiseInternal>()) {}

  static Promise<void> Resolve()
  {
    return Promise<void>(true);
  }

  static Promise<void> Reject()
  {
    return Promise<void>(false);
  }

  template <typename Callback>
  auto then(Callback callback) -> Promise<typename PromiseResultType<typename std::invoke_result<Callback>::type>::type>
  {
    using ReturnType = typename std::invoke_result<Callback>::type;
    using ResultType = typename PromiseResultType<ReturnType>::type;

    if (isSettled())
    {
      auto result = getResult();
      if (result)
      {
        if constexpr (std::is_same<ReturnType, Promise<ResultType>>::value)
        {
          return callback();
        }
        else if constexpr (std::is_same<ResultType, void>::value)
        {
          callback();
          return Promise<ResultType>::Resolve();
        }
        else
        {
          return Promise<ResultType>::Resolve(callback());
        }
      }
      else
      {
        return Promise<ResultType>::Reject();
      }
    }

    auto internal = std::get<std::shared_ptr<PromiseInternal>>(content);
    Promise<ResultType> promise;

    if constexpr (std::is_same<ReturnType, Promise<ResultType>>::value)
    {
      auto callbackWrapper = [callback, promise]()
      {
        auto innerPromise = callback();
        if constexpr (std::is_same<ResultType, void>::value)
        {
          innerPromise.then([promise]()
          {
            promise.resolve();
          });
        }
        else
        {
          innerPromise.then([promise](ResultType innerValue)
          {
            promise.resolve(innerValue);
          });
        }
      };

      internal->resolveCallbacks.push_back(callbackWrapper);
    }
    else
    {
      auto callbackWrapper = [callback, promise]()
      {
        if constexpr (std::is_same<ResultType, void>::value)
        {
          callback();
          promise.resolve();
        }
        else
        {
          promise.resolve(callback());
        }
      };

      internal->resolveCallbacks.push_back(callbackWrapper);
    }

    internal->finallyCallbacks.push_back([internal, promise]()
    {
      if (!internal->resolvedValue)
      {
        promise.reject();
      }
    });

    return promise;
  }

  template <typename Callback>
  auto finally(Callback callback) -> Promise<typename PromiseResultType<typename std::invoke_result<Callback>::type>::type>
  {
    using ReturnType = typename std::invoke_result<Callback>::type;
    using ResultType = typename PromiseResultType<ReturnType>::type;

    static_assert(std::is_same<ResultType, void>::value, "finally callback must return void");

    if (isSettled())
    {
      auto result = getResult();
      if (result)
      {
        callback();
        return Promise<ResultType>::Resolve();
      }
      else
      {
        callback();
        return Promise<ResultType>::Reject();
      }
    }

    auto internal = std::get<std::shared_ptr<PromiseInternal>>(content);
    Promise<ResultType> promise;

    if constexpr (std::is_same<ReturnType, Promise<ResultType>>::value)
    {
      auto callbackWrapper = [callback, promise]()
      {
        auto innerPromise = callback();
        innerPromise.finally([promise]()
        {
          if (innerPromise.isResolved())
          {
            promise.resolve();
          }
          else
          {
            promise.reject();
          }
        });
      };

      internal->finallyCallbacks.push_back(callbackWrapper);
    }
    else
    {
      auto callbackWrapper = [callback, promise]()
      {
        callback();
        promise.resolve();
      };

      internal->finallyCallbacks.push_back(callbackWrapper);
    }

    return promise;
  }

  void resolve() const
  {
    if (!std::holds_alternative<std::shared_ptr<PromiseInternal>>(content))
    {
      return;
    }

    auto ptr = std::get<std::shared_ptr<PromiseInternal>>(content);
    if (ptr->settled)
    {
      return;
    }

    ptr->resolvedValue = true;
    ptr->settled = true;

    for (const auto & it : ptr->resolveCallbacks)
    {
      it();
    }

    doFinally(ptr);
  }

  void reject() const
  {
    if (!std::holds_alternative<std::shared_ptr<PromiseInternal>>(content))
    {
      return;
    }

    auto ptr = std::get<std::shared_ptr<PromiseInternal>>(content);
    if (ptr->settled)
    {
      return;
    }

    ptr->settled = true;

    doFinally(ptr);
  }

  bool isResolved() const
  {
    if (auto val = std::get_if<bool>(&content))
    {
      return *val;
    }

    return std::get<std::shared_ptr<PromiseInternal>>(content)->resolvedValue;
  }

  bool isSettled() const
  {
    if (std::holds_alternative<bool>(content))
    {
      return true;
    }

    return std::get<std::shared_ptr<PromiseInternal>>(content)->settled;
  }

private:
  struct PromiseInternal
  {
    bool settled = false;
    bool resolvedValue = false;
    std::vector<CallbackInternal> resolveCallbacks;
    std::vector<FinallyCallbackInternal> finallyCallbacks;
  };

  bool getResult() const
  {
    if (auto val = std::get_if<bool>(&content))
    {
      return *val;
    }

    return std::get<std::shared_ptr<PromiseInternal>>(content)->resolvedValue;
  }

  void doFinally(const std::shared_ptr<PromiseInternal> & internal) const
  {
    for (const auto & it : internal->finallyCallbacks)
    {
      it();
    }

    internal->resolveCallbacks.clear();
    internal->finallyCallbacks.clear();
  }

  std::variant<bool, std::shared_ptr<PromiseInternal>> content;
};