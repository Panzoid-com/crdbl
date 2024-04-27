#include "OperationFilter.h"
#include <span>
#include <cstring>

OperationFilter::OperationFilter()
{
  reset();
}

OperationFilter & OperationFilter::setTagClockRange(const Tag & tag,
  const VectorTimestamp & startTime, const VectorTimestamp & endTime)
{
  tagRanges[tag] = std::make_pair(startTime, endTime);
  return *this;
}

OperationFilter & OperationFilter::setClockRange(const VectorTimestamp & startTime,
  const VectorTimestamp & endTime)
{
  clockRange = std::make_pair(startTime, endTime);
  return *this;
}

OperationFilter & OperationFilter::setSiteFilter(uint32_t siteId)
{
  siteFilter.insert(siteId);
  return *this;
}

OperationFilter & OperationFilter::setSiteFilterInvert(bool invert)
{
  siteFilterInvert = invert;
  return *this;
}

OperationFilter & OperationFilter::invert()
{
  filterInvert = !filterInvert;
  return *this;
}

OperationFilter & OperationFilter::setInvert(bool invert)
{
  filterInvert = invert;
  return *this;
}

OperationFilter & OperationFilter::reset()
{
  tagRanges.clear();
  clockRange = std::make_pair(VectorTimestamp{}, VectorTimestamp{});
  filterInvert = false;
  siteFilterInvert = false;
  siteFilter.clear();
  return *this;
}

OperationFilter & OperationFilter::boundClock(const VectorTimestamp & end)
{
  if (clockRange.second.isEmpty())
  {
    clockRange.second = end;
  }

  return *this;
}

OperationFilter & OperationFilter::boundTag(const Tag & tag, const VectorTimestamp & end)
{
  for (auto & e : tagRanges)
  {
    if (tag == e.first && e.second.second.isEmpty())
    {
      e.second.second = end;
    }
  }

  return *this;
}

OperationFilter & OperationFilter::boundTags(const VectorTimestamp & end)
{
  for (auto & e : tagRanges)
  {
    if (e.second.second.isEmpty())
    {
      e.second.second = end;
    }
  }

  return *this;
}

OperationFilter & OperationFilter::removeTag(const Tag & tag)
{
  tagRanges.erase(tag);

  return *this;
}

OperationFilter & OperationFilter::merge(const OperationFilter & other)
{
  if (!clockRange.second.isEmpty())
  {
    if (other.clockRange.second.isEmpty())
    {
      clockRange.second.reset();
    }
    else
    {
      clockRange.second.merge(other.clockRange.second);
    }
  }

  if (tagRanges.size() == 0)
  {
    //tags are unbounded; no superset exists
    return *this;
  }

  for (auto & e : other.tagRanges)
  {
    auto range = tagRanges.find(e.first);
    if (range == tagRanges.end())
    {
      tagRanges[e.first] = e.second;
    }
    else if (!range->second.second.isEmpty())
    {
      if (e.second.second.isEmpty())
      {
        range->second.second.reset();
      }
      else
      {
        range->second.second.merge(e.second.second);
      }
    }
  }

  return *this;
}

bool OperationFilter::filterByClock(const LogOperation & op) const
{
  if (clockRange.second.isEmpty())
  {
    return clockRange.first < op.ts;
  }

  return clockRange.first < op.ts && !(clockRange.second < op.ts);
}

bool OperationFilter::filterByTag(const LogOperation & op) const
{
  if (tagRanges.size() == 0)
  {
    return true;
  }

  auto it = tagRanges.find(op.tag);
  if (it != tagRanges.end())
  {
    auto & limits = it->second;
    if (limits.second.isEmpty())
    {
      return limits.first < op.ts;
    }
    else
    {
      return limits.first < op.ts && !(limits.second < op.ts);
    }
  }

  return false;
}

bool OperationFilter::filterBySite(const LogOperation & op) const
{
  if (siteFilter.size() == 0)
  {
    return !siteFilterInvert;
  }

  bool included = siteFilter.contains(op.ts.site);

  return (siteFilterInvert) ? !included : included;
}

bool OperationFilter::filter(const LogOperation & op) const
{
  if (!filterByClock(op))
  {
    return filterInvert;
  }

  if (!filterBySite(op))
  {
    return filterInvert;
  }

  if (!filterByTag(op))
  {
    return filterInvert;
  }

  return !filterInvert;
}

bool OperationFilter::isBounded() const
{
  if (!clockRange.second.isEmpty())
  {
    return true;
  }

  if (tagRanges.size() > 0)
  {
    for (auto & e : tagRanges)
    {
      if (e.second.second.isEmpty())
      {
        return false;
      }
    }

    return true;
  }

  return false;
}

bool OperationFilter::hasTags() const
{
  return tagRanges.size() > 0;
}

std::string OperationFilter::clockRangeToString() const
{
  return "({" + clockRange.first.toString() + "}, {" + clockRange.second.toString() + "}]";
}

std::string OperationFilter::toString() const
{
  std::string output;
  output += "clock: ({" + clockRange.first.toString() + "}, {" + clockRange.second.toString() + "}]\n";
  output += "tags: " + std::string((tagRanges.size() == 0) ? "none\n" : "\n");
  for (auto & item : tagRanges)
  {
    output += "  '" + item.first.toString() + "': ({" + item.second.first.toString() + "}, {" + item.second.second.toString() + "}]\n";
  }
  output += "site: " + std::string((siteFilter.size() == 0) ? "any, " : "");
  for (auto it = siteFilter.begin(); it != siteFilter.end(); ++it)
  {
    output += std::to_string(*it) + ", ";
  }
  output.pop_back();
  output.pop_back();
  output += "\n";
  output += "site invert: " + std::to_string(siteFilterInvert) + "\n";
  output += "invert: " + std::to_string(filterInvert);
  return output;
}

std::string OperationFilter::DefaultFormat()
{
  return "standard_filter_v1_bounds";
}

std::basic_string<char> OperationFilter::Serialize(const std::string & format, const OperationFilter & filter)
{
  std::basic_string<char> output;
  uint32_t length;
  output.reserve(512);

  if (format == "standard_filter_v1_bounds")
  {
    auto endVec = filter.clockRange.second.getVector();

    length = (filter.clockRange.second.isEmpty() || endVec.size() <= 1)
      ? 0 : endVec.size() - 1;
    output.append(reinterpret_cast<const char *>(&length), sizeof(uint32_t));
    if (length > 0)
    {
      output.append(
        reinterpret_cast<const char *>(filter.clockRange.second.getVector().data() + 1),
        length * sizeof(uint32_t));
    }

    for (auto & e : filter.tagRanges)
    {
      output.append(reinterpret_cast<const char *>(e.first.value.data()), sizeof(e.first.value));

      auto endVec = e.second.second.getVector();

      length = (e.second.second.isEmpty() || endVec.size() <= 1)
        ? 0 : endVec.size() - 1;
      output.append(reinterpret_cast<const char *>(&length), sizeof(uint32_t));
      if (length > 0)
      {
        output.append(
          reinterpret_cast<const char *>(endVec.data() + 1),
          length * sizeof(uint32_t));
      }
    }
  }
  else if (format == "standard_filter_v1_full")
  {
    length = filter.filterInvert;
    output.append(reinterpret_cast<const char *>(&length), sizeof(uint32_t));

    auto startVec = filter.clockRange.first.getVector();

    length = (filter.clockRange.first.isEmpty() || startVec.size() <= 1)
      ? 0 : startVec.size() - 1;
    output.append(reinterpret_cast<const char *>(&length), sizeof(uint32_t));
    if (length > 0)
    {
      output.append(
        reinterpret_cast<const char *>(startVec.data() + 1),
        length * sizeof(uint32_t));
    }

    auto endVec = filter.clockRange.second.getVector();

    length = (filter.clockRange.second.isEmpty() || endVec.size() <= 1)
      ? 0 : endVec.size() - 1;
    output.append(reinterpret_cast<const char *>(&length), sizeof(uint32_t));
    if (length > 0)
    {
      output.append(
        reinterpret_cast<const char *>(endVec.data() + 1),
        length * sizeof(uint32_t));
    }

    length = (filter.siteFilter.size() & 0x7FFFFFF) | (filter.siteFilterInvert << 31);
    output.append(reinterpret_cast<const char *>(&length), sizeof(uint32_t));
    for (auto & e : filter.siteFilter)
    {
      output.append(reinterpret_cast<const char *>(&e), sizeof(uint32_t));
    }

    for (auto & e : filter.tagRanges)
    {
      output.append(reinterpret_cast<const char *>(e.first.value.data()), sizeof(e.first.value));

      auto startVec = e.second.first.getVector();

      length = (e.second.first.isEmpty() || startVec.size() <= 1)
        ? 0 : startVec.size() - 1;
      output.append(reinterpret_cast<const char *>(&length), sizeof(uint32_t));
      if (length > 0)
      {
        output.append(
          reinterpret_cast<const char *>(startVec.data() + 1),
          length * sizeof(uint32_t));
      }

      auto endVec = e.second.second.getVector();

      length = (e.second.second.isEmpty() || endVec.size() <= 1)
        ? 0 : endVec.size() - 1;
      output.append(reinterpret_cast<const char *>(&length), sizeof(uint32_t));
      if (length > 0)
      {
        output.append(
          reinterpret_cast<const char *>(endVec.data() + 1),
          length * sizeof(uint32_t));
      }
    }
  }

  return output;
}

OperationFilter OperationFilter::Deserialize(const std::string & format, const std::string_view & data)
{
  OperationFilter output;

  uint32_t length;

  if (format == "standard_filter_v1_bounds")
  {
    std::span<const uint32_t> buffer(reinterpret_cast<const uint32_t *>(data.data()),
      data.size() / sizeof(uint32_t));

    if (buffer.begin() == buffer.end())
    {
      return output;
    }

    auto it = buffer.begin();

    length = *it;
    if (length > 0)
    {
      auto vec = std::vector<uint32_t>();
      auto span = buffer.subspan(1, length);
      vec.resize(length + 1);
      std::memcpy(vec.data() + 1, span.data(), span.size_bytes());
      output.clockRange.second = VectorTimestamp(vec);
    }
    it += length + 1;

    while (it != buffer.end())
    {
      Tag tag;
      auto tagSpan = std::span(it, it + sizeof(Tag::value) / sizeof(uint32_t));
      std::memcpy(tag.value.data(), tagSpan.data(), tagSpan.size_bytes());
      it += tagSpan.size();

      length = *it;
      if (length > 0)
      {
        auto vec = std::vector<uint32_t>();
        auto span = buffer.subspan(it - buffer.begin() + 1, length);
        vec.resize(length + 1);
        std::memcpy(vec.data() + 1, span.data(), span.size_bytes());
        output.setTagClockRange(tag, VectorTimestamp(), VectorTimestamp(vec));
      }
      else
      {
        output.setTagClockRange(tag, VectorTimestamp(), VectorTimestamp());
      }
      it += length + 1;
    }
  }
  else if (format == "standard_filter_v1_full")
  {
    std::span<const uint32_t> buffer(reinterpret_cast<const uint32_t *>(data.data()),
      data.size() / sizeof(uint32_t));

    if (buffer.begin() == buffer.end())
    {
      return output;
    }

    auto it = buffer.begin();

    output.setInvert(*it == 1);
    it++;

    length = *it;
    if (length > 0)
    {
      auto vec = std::vector<uint32_t>();
      auto span = std::span(it + 1, length);
      vec.resize(length + 1);
      std::memcpy(vec.data() + 1, span.data(), span.size_bytes());
      output.clockRange.first = VectorTimestamp(vec);
    }
    it += length + 1;

    length = *it;
    if (length > 0)
    {
      auto vec = std::vector<uint32_t>();
      auto span = std::span(it + 1, length);
      vec.resize(length + 1);
      std::memcpy(vec.data() + 1, span.data(), span.size_bytes());
      output.clockRange.second = VectorTimestamp(vec);
    }
    it += length + 1;

    length = *it;
    output.siteFilterInvert = !!(length & 0x80000000);
    length = length & 0x7FFFFFFF;
    if (length > 0)
    {
      auto span = std::span(it + 1, length);
      output.siteFilter = std::set<uint32_t>(span.begin(), span.end());
    }
    it += length + 1;

    while (it != buffer.end())
    {
      Tag tag;
      VectorTimestamp first;
      VectorTimestamp second;

      auto tagSpan = std::span(it, it + sizeof(Tag::value) / sizeof(uint32_t));
      std::memcpy(tag.value.data(), tagSpan.data(), tagSpan.size_bytes());
      it += tagSpan.size();

      length = *it;
      if (length > 0)
      {
        auto vec = std::vector<uint32_t>();
        auto span = buffer.subspan(it - buffer.begin() + 1, length);
        vec.resize(length + 1);
        std::memcpy(vec.data() + 1, span.data(), span.size_bytes());
        first = VectorTimestamp(vec);
      }
      it += length + 1;

      length = *it;
      if (length > 0)
      {
        auto vec = std::vector<uint32_t>();
        auto span = buffer.subspan(it - buffer.begin() + 1, length);
        vec.resize(length + 1);
        std::memcpy(vec.data() + 1, span.data(), span.size_bytes());
        second = VectorTimestamp(vec);
      }
      it += length + 1;

      output.setTagClockRange(tag, first, second);
    }
  }

  return output;
}