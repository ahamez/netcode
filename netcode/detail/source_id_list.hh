#pragma once

#include <boost/container/flat_set.hpp>

namespace ntc { namespace detail {

/*------------------------------------------------------------------------------------------------*/

/// @internal
/// @brief A sorted list of source identifiers.
using source_id_list = boost::container::flat_set<std::uint32_t>;

/*------------------------------------------------------------------------------------------------*/

}} // namespace ntc::detail
