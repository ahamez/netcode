#pragma once

#include <cstdint>
#include <set>

namespace ntc { namespace detail {

/*------------------------------------------------------------------------------------------------*/

/// @internal
/// @brief A sorted list of source identifiers.
using source_id_list = std::set<std::uint32_t>;

/*------------------------------------------------------------------------------------------------*/

}} // namespace ntc::detail
