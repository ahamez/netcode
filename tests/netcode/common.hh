#pragma once

#include "netcode/detail/multiple.hh"
#include "netcode/detail/source_list.hh"

namespace /* unnamed */ {

/*------------------------------------------------------------------------------------------------*/

using namespace ntc;

/*------------------------------------------------------------------------------------------------*/

// Make sure byte_buffer used in tests have a size multiple of 16.
const detail::source&
add_source(detail::source_list& sl,std::uint32_t id, detail::byte_buffer&& buf, std::size_t sz)
{
  if (buf.size() == 0 or buf.size() % 16 != 0)
  {
    buf.resize(detail::multiple(buf.size(), 16));
  return sl.emplace(id, std::move(buf), sz);
}

/*------------------------------------------------------------------------------------------------*/

} // namespace unnamed
