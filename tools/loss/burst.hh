#pragma once

#include <random>

namespace loss {

/*------------------------------------------------------------------------------------------------*/

class burst
{
public:

  burst(unsigned int good, unsigned int bad)
    : state_{state::good}
    , gen_{}
    , dist_{1, 100}
    , good_{good}
    , bad_{bad}
  {}

  /// @return true if packet should be lost.
  bool
  operator()()
  noexcept
  {
    switch (state_)
    {
        case state::good:
        {
          if (dist_(gen_) < good_)
          {
            return false; // no loss
          }
          else
          {
            state_ = state::bad;
            return true; // loss
          }
        }

        case state::bad:
        {
          if (dist_(gen_) < bad_)
          {
            return true; // loss
          }
          else
          {
            state_ = state::good;
            return false; // no loss
          }
        }

        default: __builtin_unreachable();
    }
  }

private:

  /// @brief
  enum class state {good, bad};

  /// @brief
  state state_;

  /// @brief
  std::default_random_engine gen_;

  /// @brief
  std::uniform_int_distribution<unsigned int> dist_;

  /// @brief
  unsigned int good_;

  /// @bief
  unsigned int bad_;

};

/*------------------------------------------------------------------------------------------------*/

} // namespace loss
