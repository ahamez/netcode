#pragma once

#include <chrono>
#include <limits> // numeric_limits

#include "netcode/code.hh"
#include "netcode/packetizer.hh"

namespace ntc {

/*------------------------------------------------------------------------------------------------*/

/// @brief
struct configuration
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
  std::chrono::milliseconds ack_frequency = std::chrono::milliseconds{100};

  /// @brief
  std::size_t window = std::numeric_limits<std::size_t>::max();
};

/*------------------------------------------------------------------------------------------------*/

} // namespace ntc
