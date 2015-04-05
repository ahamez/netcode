#pragma once

#include <cassert>
#include <random>

/*------------------------------------------------------------------------------------------------*/

class burst_loss
{
public:

  burst_loss(unsigned int good, unsigned int bad)
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
          if (dist_(gen_) > bad_)
          {
            return true; // loss
          }
          else
          {
            state_ = state::good;
            return false; // no loss
          }
        }
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

//class uniform_loss
//{
//public:
//
//  uniform_loss(unsigned int threshold)
//    : gen_{}
//    , dist_{1, 100}
//    , threshold_{threshold}
//  {
//    assert(threshold_ > 0 and threshold < 100);
//  }
//
//  /// @return true if packet should be lost.
//  bool
//  operator()()
//  const noexcept
//  {
//    return dist_(gen_) > threshold_;
//  }
//
//private:
//
//
//  /// @brief
//  std::default_random_engine gen_;
//
//  /// @brief
//  std::uniform_int_distribution<unsigned int> dist_;
//
//  /// @brief
//  unsigned int threshold_;
//};

/*------------------------------------------------------------------------------------------------*/
