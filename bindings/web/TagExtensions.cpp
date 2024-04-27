#include "TagExtensions.h"

Tag Tag_fromArray(val array)
{
  const auto size = array["byteLength"].as<unsigned>();
  const uint8_t * data = new uint8_t[size];

  val uint8Array = val::global("Uint8Array").new_(array["buffer"],
    array["byteOffset"], array["byteLength"]);
  val memoryView{typed_memory_view(size, data)};
  memoryView.call<void>("set", uint8Array);

  auto tag = Tag();
  if (size == Tag::SizeBytes())
  {
    memcpy(tag.value.data(), data, Tag::SizeBytes());
  }

  delete[] data;

  return tag;
}

val Tag_asArray(const Tag & tag)
{
  size_t size = tag.value.size();
  const uint32_t * data = static_cast<const uint32_t *>(tag.value.data());

  val in_arr = val(typed_memory_view(size, data));

  return in_arr;
}

Tag Tag_clone(const Tag & tag)
{
  return Tag(tag);
}

bool Tag_eqOther(const Tag & tag, const Tag & other)
{
  return tag == other;
}

void Tag_setFromOther(Tag & tag, const Tag & other)
{
  tag = other;
}

void Tag_setFromArray(Tag & tag, val array)
{
  const auto size = array["byteLength"].as<unsigned>();
  const uint8_t * data = new uint8_t[size];

  val uint8Array = val::global("Uint8Array").new_(array["buffer"],
    array["byteOffset"], array["byteLength"]);
  val memoryView{typed_memory_view(size, data)};
  memoryView.call<void>("set", uint8Array);

  if (size == Tag::SizeBytes())
  {
    memcpy(tag.value.data(), data, Tag::SizeBytes());
  }

  delete[] data;
}