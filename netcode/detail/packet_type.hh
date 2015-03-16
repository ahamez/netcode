#pragma once

namespace ntc { namespace detail {

/*------------------------------------------------------------------------------------------------*/

/// @brief Describe possible packet types.
enum class packet_type : std::uint8_t {ack = 0, repair, source};

/*------------------------------------------------------------------------------------------------*/

/// @brief Get the type of a raw packet by looking at its first byte.
inline
packet_type
get_packet_type(const char* data)
noexcept
{
  return static_cast<detail::packet_type>(data[0]);
}

/*------------------------------------------------------------------------------------------------*/

}} // namespace ntc::detail
