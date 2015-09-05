#pragma once

#include <exception>

#include "netcode/detail/visibility.hh"
#include "netcode/packet.hh"

namespace ntc {

/*------------------------------------------------------------------------------------------------*/

/// @brief Exception thrown when a packet type could not have been decoded.
/// @ingroup ntc_error
struct NTC_PUBLIC packet_type_error
  : public std::exception
{
  packet_type_error(packet  p)
    : error_packet{std::move(p)}
  {}

  packet error_packet;
};

/*------------------------------------------------------------------------------------------------*/

/// @brief Exception raised when the maximum authorized number of bytes have been read.
/// @ingroup ntc_error
struct NTC_PUBLIC overflow_error
  : public std::exception
{};

/*------------------------------------------------------------------------------------------------*/

} // namespace ntc
