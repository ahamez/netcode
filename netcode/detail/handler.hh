#pragma once

#include "netcode/galois/field.hh"
#include "netcode/types.hh"

namespace ntc { namespace detail {

/*------------------------------------------------------------------------------------------------*/

/// @internal
struct handler_base
{
  /// @brief Can delete through this base class.
  virtual ~handler_base(){}

  /// @brief Called when a packet is ready to be sent.
  virtual void on_ready_data(std::size_t nb, const char* data) = 0;
};

/*------------------------------------------------------------------------------------------------*/

/// @internal
template <typename Handler>
struct handler_derived final
  : public handler_base
{
  /// @brief Construct with a copy or a moved user handler.
  template <typename H>
  handler_derived(H&& h)
    : handler_(std::forward<H>(h))
  {}

  void
  on_ready_data(std::size_t nb, const char* data)
  override
  {
    handler_.on_ready_data(nb, data);
  }

  /// @brief The user's handler.
  Handler handler_;
};

/*------------------------------------------------------------------------------------------------*/

}} // namespace ntc::detail
