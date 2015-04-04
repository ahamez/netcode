#pragma once

#include <vector>

#include <boost/align/aligned_allocator.hpp>

namespace ntc { namespace detail {

/*------------------------------------------------------------------------------------------------*/

/// @brief A generic buffer aligned on 16 bytes.
template <typename T>
using buffer = std::vector<T, boost::alignment::aligned_allocator<T, 16>>;

/*------------------------------------------------------------------------------------------------*/

/// @internal
/// @brief An 16-aligned buffer of bytes.
/// @note Will set new bytes to 0 when resized.
using byte_buffer = std::vector<char, boost::alignment::aligned_allocator<char, 16>>;

/*------------------------------------------------------------------------------------------------*/

}} // namespace ntc::detail
