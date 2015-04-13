#pragma once

#include <chrono>
#include <limits> // numeric_limits

#include "netcode/code.hh"

namespace ntc {

/*------------------------------------------------------------------------------------------------*/

/// @brief The default configuration for ntc::encoder and ntc::decoder.
///
/// Each setting can be overwritten before passing the configuration to a ntc::encoder or
/// ntc::decoder.
struct configuration
{
  /// @brief The Galois field size is 2 ^ @p galois_field_size.
  /// @note Possible values are: 4, 8, 16 and 32.
  /// @attention When 4 or 8, length of data handled to the library can be any value. However, for
  /// size 16, length must be a multiple of 2; a multiple of 4 for size 32.
  std::size_t galois_field_size = 8;

  /// @brief Tell if the code is systematic or not.
  code code_type = code::systematic;

  /// @brief How many sources to send before a repair is generated.
  std::size_t rate = 5;

  /// @brief The frequency at which ack will be sent back from the decoder to the encoder.
  ///
  /// If 0, the user has to take care of sending ack by calling decoder::send_ack() directly.
  std::chrono::milliseconds ack_frequency = std::chrono::milliseconds{100};

  /// @brief The maximal number of sources to keep on the encoder side before discarding them.
  std::size_t window = std::numeric_limits<std::size_t>::max();
};

/*------------------------------------------------------------------------------------------------*/

} // namespace ntc
