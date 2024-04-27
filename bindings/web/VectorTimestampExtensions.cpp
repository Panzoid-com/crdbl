#include "VectorTimestampExtensions.h"

VectorTimestamp VectorTimestamp_fromArray(val array)
{
  const auto size = array["byteLength"].as<unsigned>();
  const uint8_t * data = new uint8_t[size];

  val uint8Array = val::global("Uint8Array").new_(array["buffer"],
    array["byteOffset"], array["byteLength"]);
  val memoryView{typed_memory_view(size, data)};
  memoryView.call<void>("set", uint8Array);

  auto ts = VectorTimestamp(data, size);

  delete[] data;

  return ts;
}

val VectorTimestamp_asArray(const VectorTimestamp & clock)
{
  auto vector = clock.getVector();
  size_t size = vector.size();
  const uint32_t * data = static_cast<const uint32_t *>(vector.data());

  val in_arr = val(typed_memory_view(size, data));
  // val out_arr = val::global("Uint32Array").new_(in_arr["buffer"]);

  return in_arr;
}

VectorTimestamp VectorTimestamp_clone(const VectorTimestamp & clock)
{
  return VectorTimestamp(clock);
}

bool VectorTimestamp_ltOther(const VectorTimestamp & clock, const VectorTimestamp & other)
{
  return clock < other;
}

bool VectorTimestamp_eqOther(const VectorTimestamp & clock, const VectorTimestamp & other)
{
  return clock == other;
}

void VectorTimestamp_setFromOther(VectorTimestamp & clock, const VectorTimestamp & other)
{
  clock = other;
}