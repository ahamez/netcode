#pragma once

#include "netcode/detail/source_list.hh"

namespace /* unnamed */ {

/*------------------------------------------------------------------------------------------------*/

using namespace ntc;

/*------------------------------------------------------------------------------------------------*/

// Make sure byte_buffer used in tests have a size multiple of 16.
const detail::source&
add_source(detail::source_list& sl,std::uint32_t id, detail::byte_buffer&& buf, std::size_t sz)
{
  return sl.emplace(id, std::move(buf), sz);
}

/*------------------------------------------------------------------------------------------------*/

} // namespace unnamed
