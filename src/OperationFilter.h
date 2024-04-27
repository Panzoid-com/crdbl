#pragma once
#include <unordered_map>
#include <set>
#include "Tag.h"
#include "VectorTimestamp.h"
#include "LogOperation.h"

class OperationFilter
{
public:
  OperationFilter();

  static std::string DefaultFormat();
  static std::basic_string<char> Serialize(const std::string & format, const OperationFilter & filter);
  static OperationFilter Deserialize(const std::string & format, const std::string_view & data);

  bool operator==(const OperationFilter & rhs) const = default;

  OperationFilter & setTagClockRange(const Tag & tag,
    const VectorTimestamp & startTime, const VectorTimestamp & endTime);
  OperationFilter & setClockRange(const VectorTimestamp & startTime,
    const VectorTimestamp & endTime);
  OperationFilter & setSiteFilter(uint32_t siteId);
  OperationFilter & setSiteFilterInvert(bool invert);

  OperationFilter & invert();
  OperationFilter & setInvert(bool invert);
  OperationFilter & reset();

  OperationFilter & boundClock(const VectorTimestamp & end);
  OperationFilter & boundTag(const Tag & tag, const VectorTimestamp & end);
  OperationFilter & boundTags(const VectorTimestamp & end);
  OperationFilter & removeTag(const Tag & tag);

  OperationFilter & merge(const OperationFilter & other);

  bool filter(const LogOperation & op) const;
  bool isBounded() const;
  bool hasTags() const;

  std::string clockRangeToString() const;
  std::string toString() const;

private:
  std::unordered_map<Tag, std::pair<VectorTimestamp, VectorTimestamp>> tagRanges;
  std::pair<VectorTimestamp, VectorTimestamp> clockRange;
  std::set<uint32_t> siteFilter;
  bool siteFilterInvert = false;
  bool filterInvert = false;

  bool filterByClock(const LogOperation & op) const;
  bool filterByTag(const LogOperation & op) const;
  bool filterBySite(const LogOperation & op) const;
};