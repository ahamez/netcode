#pragma once

#include "netcode/galois/field.hh"
#include "netcode/types.hh"

namespace ntc { namespace detail {

/*------------------------------------------------------------------------------------------------*/

/// @internal
class handler_base
{
public:

  virtual ~handler_base(){}

  virtual void on_ready_packet(std::size_t nb, const char* data) = 0;
};

/*------------------------------------------------------------------------------------------------*/

/// @internal
template <typename Handler>
class handler_derived
  : public handler_base
{
public:

  template <typename H>
  handler_derived(H&& h)
    : handler_(std::forward<H>(h))
  {}

  void
  on_ready_packet(std::size_t nb, const char* data)
  {
    handler_.on_ready_packet(nb, data);
  }

private:

  /// @brief The user's handler.
  Handler handler_;
};

/*------------------------------------------------------------------------------------------------*/

}} // namespace ntc::detail
