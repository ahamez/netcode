#pragma once

namespace ntc { namespace detail {

/*------------------------------------------------------------------------------------------------*/

/// @brief Round up @p value to a multiple @p mult.
inline
std::size_t
multiple(std::size_t value, std::size_t mult)
noexcept
{
  return ((value + (mult - 1)) / mult) * mult;
}

/*------------------------------------------------------------------------------------------------*/

}} // namespace ntc::detail
