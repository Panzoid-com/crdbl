#pragma once
#include <string>
#include <NodeId.h>
#include <EdgeId.h>

class IObjectSerializer
{
public:
  virtual void startObject() = 0;
  virtual void endObject() = 0;
  virtual void resumeObject() = 0;
  virtual void startArray() = 0;
  virtual void endArray() = 0;
  virtual void resumeArray() = 0;

  virtual void addPair(const std::string & key, const std::string & value) = 0;
  virtual void addPair(const std::string & key, const char * value) = 0;
  virtual void addPair(const std::string & key, const Timestamp & value) = 0;
  virtual void addPair(const std::string & key, const NodeId & value) = 0;
  virtual void addPair(const std::string & key, size_t value) = 0;
  virtual void addPair(const std::string & key, uint32_t value) = 0;
  virtual void addPair(const std::string & key, bool value) = 0;
  virtual void addPair(const std::string & key, double value) = 0;
  virtual void addPair(const std::string & key, float value) = 0;
  virtual void addPair(const std::string & key, int32_t value) = 0;
  virtual void addPair(const std::string & key, int64_t value) = 0;
  virtual void addPair(const std::string & key, int8_t value) = 0;

  virtual void addKey(const std::string & key) = 0;
  virtual void addValue(const std::string & key) = 0;
  virtual void addNullValue() = 0;

  // virtual void addValue(const std::string & value) = 0;
  // virtual void addValue(const NodeId & value) = 0;
  // virtual void addValue(const EdgeId & value) = 0;
  // virtual void addValue(size_t value) = 0;
  // virtual void addValue(uint32_t value) = 0;
  // virtual void addValue(bool value) = 0;
};