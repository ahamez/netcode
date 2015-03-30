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

  /// @brief
  ///
  /// If 0, the user will take care of sending ack by calling decoder::send_ack() directly.
  std::chrono::milliseconds ack_frequency = std::chrono::milliseconds{100};

  /// @brief
  std::size_t window = std::numeric_limits<std::size_t>::max();
};

/*------------------------------------------------------------------------------------------------*/

} // namespace ntc
