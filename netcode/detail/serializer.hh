#pragma once

#include "netcode/detail/ack.hh"
#include "netcode/detail/repair.hh"
#include "netcode/detail/source.hh"
#include "netcode/detail/types.hh"

namespace ntc { namespace detail {

/*------------------------------------------------------------------------------------------------*/

/// @internal
struct serializer
{
  virtual void operator()(const ack&   , const std::function<on_ready_packet_fn>&) const = 0;
  virtual void operator()(const repair&, const std::function<on_ready_packet_fn>&) const = 0;
  virtual void operator()(const source&, const std::function<on_ready_packet_fn>&) const = 0;
};

/*------------------------------------------------------------------------------------------------*/

}} // namespace ntc::detail
