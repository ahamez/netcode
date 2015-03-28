#pragma once

#include <limits> // numeric_limits

#include "netcode/code.hh"
#include "netcode/packetizer.hh"

namespace ntc {

/*------------------------------------------------------------------------------------------------*/

/// @brief
struct default_configuration
{
  /// @brief
  std::size_t galois_field_size = 8;

  /// @brief
  code code_type = code::systematic;

  /// @brief
  packetizer packetizer_type = packetizer::simple;

  /// @brief
  std::size_t rate = 5;

  /// @brief
  std::size_t ack_frequency = 100; /*ms*/

  /// @brief
  std::size_t window = std::numeric_limits<std::size_t>::max();
};

/*------------------------------------------------------------------------------------------------*/

} // namespace ntc
