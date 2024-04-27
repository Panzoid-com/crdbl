#include <IObjectSerializer.h>
#include <Core.h>
#include <string>
#include <stack>
#include <sstream>
#include <type_traits>

class JsonSerializer : public IObjectSerializer
{
public:
  JsonSerializer() {}

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

  std::string result();
private:
  std::stack<std::streampos> objectStartStack;
  std::ostringstream output;
};