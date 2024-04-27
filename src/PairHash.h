#include <utility>

struct PairHash
{
  template <class T1, class T2>
  std::size_t operator() (const std::pair<T1, T2> &pair) const
  {
    size_t res = 17;
    res = res * 31 + std::hash<T1>()(pair.first);
    res = res * 31 + std::hash<T2>()(pair.second);
    return res;
  }
};