#pragma once

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-parameter"
#include <boost/container/flat_set.hpp>
#pragma GCC diagnostic pop

namespace ntc { namespace detail {

/*------------------------------------------------------------------------------------------------*/

/// @internal
/// @brief A sorted list of source identifiers.
using source_id_list = boost::container::flat_set<std::uint32_t>;

/*------------------------------------------------------------------------------------------------*/

}} // namespace ntc::detail
