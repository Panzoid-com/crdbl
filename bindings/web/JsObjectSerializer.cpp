#include "JsObjectSerializer.h"

void JsObjectSerializer::startValue(val value, bool push)
{
  if (ctxStack.size() > 0)
  {
    if (ctxStack.back().isString())
    {
      val key = ctxStack.back();
      ctxStack.pop_back();

      if (ctxStack.size() > 0)
      {
        ctxStack.back().set(key, value);
      }
    }
    else if (ctxStack.back().isArray())
    {
      ctxStack.back().call<void>("push", value);
    }
  }

  if (ctxStack.size() == 0)
  {
    resultVal = value;
  }

  if (push)
  {
    ctxStack.push_back(value);
  }
}

void JsObjectSerializer::startObject()
{
  val newObj = val::global("Object").new_();
  startValue(newObj, true);
}

void JsObjectSerializer::endObject()
{
  lastPopped = ctxStack.back();
  ctxStack.pop_back();
}

void JsObjectSerializer::resumeObject()
{
  if (lastPopped.isNull() || lastPopped.isArray())
  {
    return;
  }

  ctxStack.push_back(lastPopped);
  lastPopped = val::null();
}

void JsObjectSerializer::startArray()
{
  val newArr = val::global("Array").new_();
  startValue(newArr, true);
}

void JsObjectSerializer::endArray()
{
  lastPopped = ctxStack.back();
  ctxStack.pop_back();
}

void JsObjectSerializer::resumeArray()
{
  if (lastPopped.isNull() || !lastPopped.isArray())
  {
    return;
  }

  ctxStack.push_back(lastPopped);
  lastPopped = val::null();
}

void JsObjectSerializer::addPair(const std::string & key, bool value)
{
  ctxStack.back().set(key.c_str(), val(value));
}

void JsObjectSerializer::addPair(const std::string & key, const std::string & value)
{
  ctxStack.back().set(key.c_str(), val(value));
}

void JsObjectSerializer::addPair(const std::string & key, const char * value)
{
  ctxStack.back().set(key.c_str(), val(value));
}

void JsObjectSerializer::addPair(const std::string & key, const Timestamp & value)
{
  ctxStack.back().set(key.c_str(), val(value));
}

void JsObjectSerializer::addPair(const std::string & key, const NodeId & value)
{
  ctxStack.back().set(key.c_str(), val(value.toString()));
}

void JsObjectSerializer::addPair(const std::string & key, size_t value)
{
  ctxStack.back().set(key.c_str(), val(value));
}

void JsObjectSerializer::addPair(const std::string & key, uint32_t value)
{
  ctxStack.back().set(key.c_str(), val(value));
}

void JsObjectSerializer::addPair(const std::string & key, double value)
{
  ctxStack.back().set(key.c_str(), val(value));
}

void JsObjectSerializer::addPair(const std::string & key, float value)
{
  ctxStack.back().set(key.c_str(), val(value));
}

void JsObjectSerializer::addPair(const std::string & key, int32_t value)
{
  ctxStack.back().set(key.c_str(), val(value));
}

void JsObjectSerializer::addPair(const std::string & key, int64_t value)
{
  ctxStack.back().set(key.c_str(), val(value));
}

void JsObjectSerializer::addPair(const std::string & key, int8_t value)
{
  ctxStack.back().set(key.c_str(), val(value));
}

void JsObjectSerializer::addKey(const std::string & key)
{
  ctxStack.push_back(val(key.c_str()));
}

void JsObjectSerializer::addValue(const std::string & value)
{
  startValue(val(value), false);
}

void JsObjectSerializer::addNullValue()
{
  startValue(val::null(), false);
}

val JsObjectSerializer::result()
{
  return resultVal;
}