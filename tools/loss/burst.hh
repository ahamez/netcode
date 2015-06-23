#pragma once

#include <random>

namespace loss {

/*------------------------------------------------------------------------------------------------*/

class burst
{
public:

  burst(unsigned int good, unsigned int bad)
    : m_state{state::good}
    , m_gen{}
    , m_dist{1, 100}
    , m_good{good}
    , m_bad{bad}
  {}

  /// @return true if packet should be lost.
  bool
  operator()()
  noexcept
  {
    switch (m_state)
    {
        case state::good:
        {
          if (m_dist(m_gen) < m_good)
          {
            return false; // no loss
          }
          else
          {
            m_state = state::bad;
            return true; // loss
          }
        }

        case state::bad:
        {
          if (m_dist(m_gen) < m_bad)
          {
            return true; // loss
          }
          else
          {
            m_state = state::good;
            return false; // no loss
          }
        }

        default: __builtin_unreachable();
    }
  }

private:

  enum class state {good, bad};
  state m_state;
  std::default_random_engine m_gen;
  std::uniform_int_distribution<unsigned int> m_dist;
  unsigned int m_good;
  unsigned int m_bad;
};

/*------------------------------------------------------------------------------------------------*/

} // namespace loss
