#pragma once

#include <cassert>
#include <random>

namespace loss {

/*------------------------------------------------------------------------------------------------*/

class uniform
{
public:

  uniform(unsigned int threshold)
    : gen_{}
    , dist_{1, 100}
    , threshold_{threshold}
  {
    assert(threshold_ > 0 and threshold < 100);
  }

  /// @return true if packet should be lost.
  bool
  operator()()
  noexcept
  {
    return dist_(gen_) > threshold_;
  }

private:


  /// @brief
  std::default_random_engine gen_;

  /// @brief
  std::uniform_int_distribution<unsigned int> dist_;

  /// @brief
  unsigned int threshold_;
};

/*------------------------------------------------------------------------------------------------*/

} // namespace loss
