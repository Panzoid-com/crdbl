#include <IObjectSerializer.h>
#include <emscripten/bind.h>
#include <Core.h>
#include <deque>
#include <string>
#include <type_traits>

using namespace emscripten;

class JsObjectSerializer : public IObjectSerializer
{
public:
  JsObjectSerializer()
    : resultVal(val::undefined()), lastPopped(val::undefined())
  {

  }

  void startObject();
  void endObject();
  void resumeObject();

  void startArray();
  void endArray();
  void resumeArray();

  void addPair(const std::string & key, bool value);
  void addPair(const std::string & key, const std::string & value);
  void addPair(const std::string & key, const char * value);
  void addPair(const std::string & key, const Timestamp & value);
  void addPair(const std::string & key, const NodeId & value);
  void addPair(const std::string & key, size_t value);
  void addPair(const std::string & key, uint32_t value);
  void addPair(const std::string & key, double value);
  void addPair(const std::string & key, float value);
  void addPair(const std::string & key, int32_t value);
  void addPair(const std::string & key, int64_t value);
  void addPair(const std::string & key, int8_t value);

  void addKey(const std::string & key);
  void addValue(const std::string & value);
  void addNullValue();

  val result();

private:
  void startValue(val value, bool push);

  val resultVal = val::null();
  val lastPopped = val::null();
  std::deque<val> ctxStack;
};