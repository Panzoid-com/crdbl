#include "OperationFilterWrapper.h"

val OperationFilter_Serialize(const std::string & format, const OperationFilter & filter)
{
  auto result = OperationFilter::Serialize(format, filter);

  val in_arr = val(typed_memory_view(result.size(), result.data()));
  val out_arr = val::global("Uint8Array").new_(in_arr);

  return out_arr;
}

OperationFilter OperationFilter_Deserialize(const std::string & format, val arrayBufferView)
{
  const auto size = arrayBufferView["byteLength"].as<unsigned>();
  const uint8_t * data = new uint8_t[size];

  val uint8Array = val::global("Uint8Array").new_(arrayBufferView["buffer"],
    arrayBufferView["byteOffset"], arrayBufferView["byteLength"]);
  val memoryView{typed_memory_view(size, data)};
  memoryView.call<void>("set", uint8Array);

  auto filter = OperationFilter::Deserialize(format,
    std::string_view(reinterpret_cast<const char *>(data), size));

  delete[] data;

  return filter;
}

void OperationFilter_setTagClockRange(OperationFilter & ref,
  const Tag * tag, const VectorTimestamp * startTime, const VectorTimestamp * endTime)
{
  Tag defaultTag;
  if (tag == nullptr)
  {
    tag = &defaultTag;
  }

  VectorTimestamp emptyClock;
  if (startTime == nullptr)
  {
    startTime = &emptyClock;
  }
  if (endTime == nullptr)
  {
    endTime = &emptyClock;
  }

  ref.setTagClockRange(*tag, *startTime, *endTime);
}

void OperationFilter_setClockRange(OperationFilter & ref,
  const VectorTimestamp * startTime, const VectorTimestamp * endTime)
{
  VectorTimestamp empty;
  if (startTime == nullptr)
  {
    startTime = &empty;
  }
  if (endTime == nullptr)
  {
    endTime = &empty;
  }

  ref.setClockRange(*startTime, *endTime);
}

void OperationFilter_setSiteFilter(OperationFilter & ref, uint32_t siteId)
{
  ref.setSiteFilter(siteId);
}

void OperationFilter_setSiteFilterInvert(OperationFilter & ref, bool invert)
{
  ref.setSiteFilterInvert(invert);
}

void OperationFilter_invert(OperationFilter & ref)
{
  ref.invert();
}

void OperationFilter_reset(OperationFilter & ref)
{
  ref.reset();
}

void OperationFilter_boundClock(OperationFilter & ref, const VectorTimestamp & clock)
{
  ref.boundClock(clock);
}
void OperationFilter_boundTag(OperationFilter & ref, const Tag & tag, const VectorTimestamp & clock)
{
  ref.boundTag(tag, clock);
}
void OperationFilter_boundTags(OperationFilter & ref, const VectorTimestamp & clock)
{
  ref.boundTags(clock);
}
void OperationFilter_removeTag(OperationFilter & ref, const Tag & tag)
{
  ref.removeTag(tag);
}

void OperationFilter_merge(OperationFilter & ref, const OperationFilter & other)
{
  ref.merge(other);
}

//NOTE: deprecated; use streams
//  may eventually replace with a LogOperation reference option
bool OperationFilter_filter(const OperationFilter & ref, val operation)
{
  const auto size = operation["byteLength"].as<unsigned>();
  const uint8_t * data = new uint8_t[size];

  val uint8Array = val::global("Uint8Array").new_(operation["buffer"]);
  val memoryView{typed_memory_view(size, data)};
  memoryView.call<void>("set", uint8Array);

  bool result = ref.filter(*reinterpret_cast<const LogOperation *>(data));

  delete[] data;
  
  return result;
}

OperationFilter OperationFilter_clone(const OperationFilter & other)
{
  return OperationFilter(other);
}

void OperationFilter_setFromOther(OperationFilter & ref, const OperationFilter & other)
{
  ref = other;
}

OperationFilter OperationFilter_Create()
{
  return OperationFilter();
}