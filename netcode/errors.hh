#pragma once

#include <exception>

namespace ntc {

/*------------------------------------------------------------------------------------------------*/

/// @brief Exception thrown when a packet type could not have been decoded.
/// @ingroup ntc_error
struct packet_type_error
  : public std::exception
{};

/*------------------------------------------------------------------------------------------------*/

/// @brief Exception raised when the maximum authorized number of bytes have been read.
/// @ingroup ntc_error
struct overflow_error
  : public std::exception
{};

/*------------------------------------------------------------------------------------------------*/

} // namespace ntc
