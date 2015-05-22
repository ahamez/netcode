#pragma once

#include <exception>

namespace ntc {

/*------------------------------------------------------------------------------------------------*/

/// @brief Exception thrown when a packet type could not have been decoded.
struct packet_type_error
  : public std::exception
{};

/*------------------------------------------------------------------------------------------------*/

/// @brief Exception raised
struct overflow_error
  : public std::exception
{};

/*------------------------------------------------------------------------------------------------*/

} // namespace ntc
