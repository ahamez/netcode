#pragma once

#include <memory> // unique_ptr

#include "netcode/detail/protocol/simple.hh"
#include "netcode/detail/serializer.hh"
#include "netcode/protocol.hh"

namespace ntc { namespace detail {

/*------------------------------------------------------------------------------------------------*/

/// @brief Create a protocol to de/serialize packets.
inline
std::unique_ptr<detail::serializer>
mk_protocol(ntc::protocol p, handler_base& h)
{
  switch (p)
  {
    case ntc::protocol::simple :
      return std::unique_ptr<detail::serializer>{new detail::protocol::simple{h}};

    default: __builtin_unreachable();
  }
}

/*------------------------------------------------------------------------------------------------*/

}} // namespace ntc::detail
