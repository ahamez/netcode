#pragma once

namespace ntc { namespace detail {

/*------------------------------------------------------------------------------------------------*/

inline
std::size_t
make_multiple(std::size_t value, std::size_t mult)
{
  return ((value + (mult - 1)) / mult) * mult;
}

/*------------------------------------------------------------------------------------------------*/

}} // namespace ntc::detail
