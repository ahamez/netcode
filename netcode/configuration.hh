#pragma once

#include <chrono>
#include <limits> // numeric_limits

#include "netcode/code.hh"

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
  std::size_t rate = 5;

  /// @brief Indicate that the user will take care of sending ack.
  bool manual_ack_send = false;

  /// @brief
  std::chrono::milliseconds ack_frequency = std::chrono::milliseconds{100};

  /// @brief
  std::size_t window = std::numeric_limits<std::size_t>::max();
};

/*------------------------------------------------------------------------------------------------*/

} // namespace ntc
