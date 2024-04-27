#pragma once
#include "Timestamp.h"
#include "Effect.h"
#include <forward_list>

template <class T>
struct Data
{
  Timestamp id;
  Effect effect;
  T value;
};

template <class T>
class Value
{
public:
  using ChangedCallback = std::function<void(const T &, const T &)>;

  constexpr static size_t valueSize() { return sizeof(T); }

  T getValue() const;
  T getValue(const Timestamp & timestamp) const;
  bool isDefined() const;
  bool isDefinedAt(const Timestamp & timestamp) const;
  bool isModified() const;
  bool isModifiedSince(const Timestamp & timestamp) const;
  void setValue(const Timestamp & timestamp, T value);
  void setValue(const Timestamp & timestamp, T value, ChangedCallback callback);
  void setValue(const Timestamp & timestamp, const uint8_t * value, size_t length, ChangedCallback callback);
  void updateEffect(const Timestamp & timestamp, int delta);
  void updateEffect(const Timestamp & timestamp, int delta, ChangedCallback callback);
  void deinitializeValue(const Timestamp & timestamp, ChangedCallback callback);
  void deleteValue(const Timestamp & timestamp, ChangedCallback callback);

  std::string toString() const;
  std::string toString(const T & value) const;
private:
  std::forward_list<Data<T>> children;

  const Data<T> * getData() const;
  const Data<T> * getData(typename std::forward_list<Data<T>>::const_iterator start) const;
  const Data<T> * getData(const Timestamp & timestamp) const;
  Data<T> * getChild(const Timestamp & timestamp);
  typename std::forward_list<Data<T>>::iterator findPreviousChild(const Timestamp & timestamp);
  void compareAndCallIfChanged(const Data<T> * newData, const Data<T> * oldData, const ChangedCallback & callback) const;
};