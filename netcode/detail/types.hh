#pragma once

#include <cstdint>

namespace ntc { namespace detail {

/*------------------------------------------------------------------------------------------------*/

/// @todo Ensure that the symbol is aligned on 16 bytes with a specific allocator.
using symbol_buffer_type = std::vector<char>;

/*------------------------------------------------------------------------------------------------*/

enum class packet_type : std::uint8_t {ack = 0, repair, source};

/*------------------------------------------------------------------------------------------------*/

}} // namespace ntc::detail
