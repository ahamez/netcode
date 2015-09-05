#pragma once

#include "netcode/detail/visibility.hh"

namespace ntc {

/*------------------------------------------------------------------------------------------------*/

/// @brief Describe if a decoder gives data in order or not
/// @see decoder::decoder
/// @ingroup ntc_decoder
enum class NTC_PUBLIC in_order {yes, no};

/*------------------------------------------------------------------------------------------------*/

} // namespace ntc
