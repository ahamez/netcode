#pragma once

#include "netcode/errors.hh"

namespace ntc { namespace detail {

/*------------------------------------------------------------------------------------------------*/

/// @brief Describe possible packet types.
enum class packet_type : std::uint8_t {ack = 0, repair = 1, source = 2};

/*------------------------------------------------------------------------------------------------*/

/// @brief Get the type of a raw packet by looking at its first byte.
/// @throw packet_type_error if the type could not have been read.
inline
packet_type
get_packet_type(const char* data)
{
  const auto ty = *reinterpret_cast<const std::uint8_t*>(data);
  switch (ty)
  {
    case 0:
      return packet_type::ack;

    case 1:
      return packet_type::repair;

    case 2:
      return packet_type::source;

    default:
      throw packet_type_error{};
  }
}

/*------------------------------------------------------------------------------------------------*/

}} // namespace ntc::detail
