#pragma once

#include <algorithm> // lower_bound
#include <vector>

namespace ntc { namespace detail {

/*------------------------------------------------------------------------------------------------*/

/// @internal
/// @brief A list of source identifiers.
using source_id_list = std::vector<std::uint32_t>;

/*------------------------------------------------------------------------------------------------*/

/// @internal
/// @brief Insert an identifier in a list of identifiers, keeping it sorted.
inline
void
insertion_sort(source_id_list& ids, std::uint32_t id)
{
  const auto ids_end = end(ids);
  // Return an iterator to the first element that is greater or equal than id.
  const auto lb = std::lower_bound(begin(ids), ids_end, id);
  // Check if id doesn't exist in ids.
  if ((lb != ids_end and *lb != id) or (lb == ids_end))
  {
    ids.insert(lb, id);
  }
}

/*------------------------------------------------------------------------------------------------*/

}} // namespace ntc::detail
