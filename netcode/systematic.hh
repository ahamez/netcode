#pragma once

#include "netcode/detail/visibility.hh"

namespace ntc {

/*------------------------------------------------------------------------------------------------*/

/// @brief Describe if a code is systematic or not.
/// @see encoder::set_code_type
/// @see encoder::code_type
/// @ingroup ntc_encoder
enum class NTC_PUBLIC systematic {yes, no};

/*------------------------------------------------------------------------------------------------*/

} // namespace ntc
