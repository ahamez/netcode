#pragma once

#include <vector>

#include "netcode/detail/allocator.hh"

namespace ntc { namespace detail {

/*------------------------------------------------------------------------------------------------*/

/// @brief An aligned buffer of bytes.
using symbol_buffer_type = std::vector<char, default_init_aligned_alloc<char, 16>>;

/*------------------------------------------------------------------------------------------------*/

/// @brief Describe possible packet types.
enum class packet_type : std::uint8_t {ack = 0, repair, source};

/*------------------------------------------------------------------------------------------------*/

}} // namespace ntc::detail
