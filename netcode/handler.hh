#pragma once

#include <functional> // function

namespace ntc {

/*------------------------------------------------------------------------------------------------*/

/// @brief The type of a user's handler.
///
/// When used as an handler for data (that is, for data ready to be sent on the network), an handler
/// will be called repetitively until all data was handed. A nullptr and a size of 0 as parameters
/// will mark the end of the data.
///
/// When used as an handler for a symbol on the decoder side, the symbol will be handed in only one
/// call.
using handler = std::function<void(const char*, std::size_t)>;

/*------------------------------------------------------------------------------------------------*/

} // namespace ntc
