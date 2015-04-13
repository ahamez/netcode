#pragma once

#include "netcode/c/handlers.h"

namespace ntc { namespace detail {

/*------------------------------------------------------------------------------------------------*/

/// @internal
/// @brief Wrap C callbacks.
struct c_packet_handler
{
  ntc_packet_handler handler;

  void
  operator()(const char* data, std::size_t sz)
  noexcept
  {
    handler.prepare_packet(handler.context, data, sz);
  }

  void
  operator()()
  noexcept
  {
    handler.send_packet(handler.context);
  }
};

/*------------------------------------------------------------------------------------------------*/

/// @internal
/// @brief Wrap C callbacks.
struct c_data_handler
{
  ntc_data_handler handler;

  void
  operator()(const char* data, std::size_t sz)
  noexcept
  {
    handler.read_data(handler.context, data, sz);
  }
};

/*------------------------------------------------------------------------------------------------*/

}} // namespace ntc::detail
