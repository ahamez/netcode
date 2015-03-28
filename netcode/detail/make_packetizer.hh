#pragma once

#include <memory> // unique_ptr

#include "netcode/detail/packetizer_base.hh"
#include "netcode/detail/packetizer_simple.hh"
#include "netcode/packetizer.hh"

namespace ntc { namespace detail {

/*------------------------------------------------------------------------------------------------*/

/// @brief Create a packetizer to de/serialize packets.
inline
std::unique_ptr<packetizer_base>
make_packetizer(packetizer p, handler_base& h)
{
  switch (p)
  {
    case packetizer::simple :
      return std::unique_ptr<packetizer_base>{new packetizer_simple{h}};

    default: __builtin_unreachable();
  }
}

/*------------------------------------------------------------------------------------------------*/

}} // namespace ntc::detail
