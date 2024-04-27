#include "Value.h"
#include "NodeId.h"
#include "Json.h"

template <class T>
const Data<T> * Value<T>::getData() const
{
  return getData(children.begin());
}

template <class T>
const Data<T> * Value<T>::getData(typename std::forward_list<Data<T>>::const_iterator start) const
{
  for (auto it = start; it != children.end(); ++it)
  {
    if (it->effect.isVisible())
    {
      return &(*it);
    }
  }

  return nullptr;
}

template <class T>
const Data<T> * Value<T>::getData(const Timestamp & timestamp) const
{
  for (const auto & data : children)
  {
    if (data.effect.isVisible() && (data.id < timestamp || data.id == timestamp))
    {
      return &data;
    }
  }

  return nullptr;
}

template <class T>
T Value<T>::getValue() const
{
  const Data<T> * data = getData();

  if (data != nullptr)
  {
    return data->value;
  }

  return T();
}

template <class T>
T Value<T>::getValue(const Timestamp & timestamp) const
{
  const Data<T> * data = getData(timestamp);

  if (data != nullptr)
  {
    return data->value;
  }

  return T();
}

template <class T>
bool Value<T>::isDefined() const
{
  return getData() != nullptr;
}

template <class T>
bool Value<T>::isDefinedAt(const Timestamp & timestamp) const
{
  return getData(timestamp) != nullptr;
}

template <class T>
bool Value<T>::isModified() const
{
  return getValue() != T();
}

template <class T>
bool Value<T>::isModifiedSince(const Timestamp & timestamp) const
{
  return getValue() != getValue(timestamp);
}

template <class T>
void Value<T>::setValue(const Timestamp & timestamp, T value)
{
  setValue(timestamp, value, [](const T &, const T &){});
}

template <class T>
void Value<T>::setValue(const Timestamp & timestamp, const uint8_t * value,
  size_t length, ChangedCallback callback)
{
  if (length == 0)
  {
    deleteValue(timestamp, callback);
    return;
  }
  if (length != valueSize())
  {
    return;
  }

  setValue(timestamp, *reinterpret_cast<const T *>(value), callback);
}

template <class T>
void Value<T>::setValue(const Timestamp & timestamp, T value,
  ChangedCallback callback)
{
  const Data<T> * oldValue = getData();
  Data<T> * data = getChild(timestamp);
  T prevDataValue = data->value;

  data->value = value;
  data->effect.initialize();

  const Data<T> * newValue = getData();
  if (newValue != oldValue)
  {
    compareAndCallIfChanged(newValue, oldValue, callback);
  }
  else if (prevDataValue != value)
  {
    callback(value, prevDataValue);
  }
}

template <class T>
void Value<T>::updateEffect(const Timestamp & timestamp, int delta)
{
  updateEffect(timestamp, delta, [](const T &, const T &){});
}

template <class T>
void Value<T>::updateEffect(const Timestamp & timestamp, int delta,
  ChangedCallback callback)
{
  const Data<T> * oldValue = getData();
  Data<T> * data = getChild(timestamp);

  data->effect += delta;

  const Data<T> * newValue = getData();
  if (newValue != oldValue)
  {
    compareAndCallIfChanged(newValue, oldValue, callback);
  }
}

template <class T>
void Value<T>::deinitializeValue(const Timestamp & timestamp, ChangedCallback callback)
{
  Data<T> * data = getChild(timestamp);
  const Data<T> * currentValue = getData();

  data->effect.deinitialize();

  if (currentValue == data)
  {
    const Data<T> * newValue = getData();
    compareAndCallIfChanged(newValue, currentValue, callback);
  }
}

template <class T>
void Value<T>::deleteValue(const Timestamp & timestamp, ChangedCallback callback)
{
  Data<T> * currentValue = nullptr;
  auto it = children.before_begin();
  for (auto next = children.begin(); next != children.end(); ++next)
  {
    if (currentValue == nullptr && next->effect.isVisible())
    {
      currentValue = &*next;
    }

    if (next->id == timestamp)
    {
      Data<T> * data = &*next;
      T oldValue = next->value;
      auto newBegin = children.erase_after(it);
      if (currentValue == data)
      {
        const Data<T> * newData = getData(newBegin);
        T newValue = newData != nullptr ? newData->value : T();
        if (newValue != oldValue)
        {
          callback(newValue, oldValue);
        }
      }

      return;
    }

    it = next;
  }
}

template <class T>
Data<T> * Value<T>::getChild(const Timestamp & timestamp)
{
  auto it = findPreviousChild(timestamp);
  if (it == children.before_begin() || !(it->id == timestamp))
  {
    return &*children.insert_after(it, {timestamp, Effect(), T()});
  }

  return &*it;
}

template <class T>
typename std::forward_list<Data<T>>::iterator Value<T>::findPreviousChild(const Timestamp & timestamp)
{
  auto it = children.before_begin();
  for (auto next = children.begin(); next != children.end(); ++next)
  {
    if (next->id < timestamp)
    {
      break;
    }

    it = next;
  }

  return it;
}

template <class T>
void Value<T>::compareAndCallIfChanged(const Data<T> * newData,
  const Data<T> * oldData, const ChangedCallback & callback) const
{
  if (oldData == newData)
  {
    __builtin_unreachable();
  }

  if (oldData == nullptr)
  {
    if (newData->value != T())
    {
      callback(newData->value, T());
    }
  }
  else if (newData == nullptr)
  {
    if (oldData->value != T())
    {
      callback(T(), oldData->value);
    }
  }
  else if (newData->value != oldData->value)
  {
    callback(newData->value, oldData->value);
  }
}

template <class T>
std::string Value<T>::toString() const
{
  std::string outString;
  outString += toString(getValue());
  return outString;
}

template <class T>
std::string Value<T>::toString(const T & value) const
{
  std::string outString;
  outString += std::to_string(value);
  return outString;
}

template <>
std::string Value<double>::toString(const double & value) const
{
  std::string outString;
  outString += DoubleToString(value);
  return outString;
}

template class Value<bool>;
template class Value<int32_t>;
template class Value<int64_t>;
template class Value<float>;
template class Value<double>;
template class Value<int8_t>;