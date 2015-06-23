#pragma once

#include <cassert>
#include <random>

namespace loss {

/*------------------------------------------------------------------------------------------------*/

class uniform
{
public:

  explicit uniform(unsigned int threshold)
    : m_gen{}
    , m_dist{1, 100}
    , m_threshold{threshold}
  {
    assert(threshold > 0 and threshold < 100);
  }

  /// @return true if packet should be lost.
  bool
  operator()()
  noexcept
  {
    return m_dist(m_gen) > m_threshold;
  }

private:

  std::default_random_engine m_gen;
  std::uniform_int_distribution<unsigned int> m_dist;
  unsigned int m_threshold;
};

/*------------------------------------------------------------------------------------------------*/

} // namespace loss
