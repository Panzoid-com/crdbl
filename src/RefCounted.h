#pragma once
#include <stdexcept>

template <class T>
class RefCounted
{
public:
  RefCounted() : ptr(nullptr), count(nullptr) {}
  explicit RefCounted(T * ptr) : ptr(ptr), count(nullptr) {}
  RefCounted(const RefCounted<T> & rhs)
    : ptr(rhs.ptr), count(rhs.count)
  {
    if (ptr == nullptr)
    {
      return;
    }
    if (!count)
    {
      //hack: allow mutating the rhs in a const copy constructor
      //  allowable since the behavior is exactly the same as if the count
      //  was not lazily allocated; this just facilitates the optimization
      count = const_cast<RefCounted<T> *>(&rhs)->count = new uint8_t(1);
    }
    else
    {
      ++*count;
    }
  }
  RefCounted(RefCounted<T> && rhs)
    : ptr(rhs.ptr), count(rhs.count)
  {
    rhs.ptr = nullptr;
  }
  ~RefCounted()
  {
    if (ptr == nullptr)
    {
      return;
    }
    if (count == nullptr)
    {
      //we assume that ptr was allocated with new[] (in our cases, it always is)
      //not that this is great regardless
      delete_ptr: delete[] ptr;
    }
    else if (*count == 0)
    {
      delete count;
      goto delete_ptr; //code size optimization
    }
    else
    {
      --*count;
    }
  }

  RefCounted<T> const & operator=(RefCounted<T> && rhs)
  {
    if (ptr != nullptr)
    {
      if (count == nullptr)
      {
        delete[] ptr;
      }
      else if (*count == 0)
      {
        delete count;
        delete[] ptr;
      }
      else
      {
        --*count;
      }
    }

    ptr = rhs.ptr;
    count = rhs.count;
    rhs.ptr = nullptr;
    return *this;
  }

  T * operator->() { return ptr; }
  const T * operator->() const { return ptr; }

  T & operator*() { return *ptr; }
  const T & operator*() const { return *ptr; }

  bool operator==(const RefCounted<T> & rhs) const
  {
    return ptr == rhs.ptr;
  }

  bool operator==(const T * rhs) const
  {
    return ptr == rhs;
  }

  T * release()
  {
    if (count != nullptr)
    {
      if (*count != 0)
      {
        //there are other references that still exist
        throw std::runtime_error("Cannot release a reference with other references still existing");
      }
      delete count;
    }

    ptr = nullptr;

    return ptr;
  }

private:
  T * ptr;
  uint8_t * count;
};