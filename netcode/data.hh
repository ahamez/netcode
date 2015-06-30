#pragma once

#include "netcode/detail/buffer.hh"

namespace ntc {

/*------------------------------------------------------------------------------------------------*/

/// @ingroup ntc_data
/// @brief The type to hold application data
///
/// It's a std::vector<char> with a special allocator to make sure some requirements are fulfilled.
using data = detail::byte_buffer;

/*------------------------------------------------------------------------------------------------*/

} // namespace ntc
