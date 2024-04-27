#include <emscripten/bind.h>
#include <OperationFilter.h>
#include <VectorTimestamp.h>
#include <Tag.h>
#include <string>

using namespace emscripten;

val OperationFilter_Serialize(const std::string & format, const OperationFilter & filter);
OperationFilter OperationFilter_Deserialize(const std::string & format, val arrayBufferView);

void OperationFilter_setTagClockRange(OperationFilter & ref,
  const Tag * tag, const VectorTimestamp * startTime = nullptr,
  const VectorTimestamp * endTime = nullptr);
void OperationFilter_setClockRange(OperationFilter & ref,
  const VectorTimestamp * startTime, const VectorTimestamp * endTime = nullptr);
void OperationFilter_setSiteFilter(OperationFilter & ref, uint32_t siteId);
void OperationFilter_setSiteFilterInvert(OperationFilter & ref, bool invert);
void OperationFilter_invert(OperationFilter & ref);
void OperationFilter_reset(OperationFilter & ref);

void OperationFilter_boundClock(OperationFilter & ref, const VectorTimestamp & clock);
void OperationFilter_boundTag(OperationFilter & ref, const Tag & tag, const VectorTimestamp & clock);
void OperationFilter_boundTags(OperationFilter & ref, const VectorTimestamp & clock);
void OperationFilter_removeTag(OperationFilter & ref, const Tag & tag);

void OperationFilter_merge(OperationFilter & ref, const OperationFilter & other);

bool OperationFilter_filter(const OperationFilter & ref, val operation);

OperationFilter OperationFilter_clone(const OperationFilter & other);
void OperationFilter_setFromOther(OperationFilter & ref, const OperationFilter & other);
OperationFilter OperationFilter_Create();