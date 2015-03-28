#pragma once

#include "netcode/detail/ack.hh"
#include "netcode/detail/handler.hh"
#include "netcode/detail/repair.hh"
#include "netcode/detail/source.hh"

namespace ntc { namespace detail {

/*------------------------------------------------------------------------------------------------*/

/// @internal
/// @brief The base class for any packetizer of @ref ack, @ref repair and @ref source.
class packetizer_base
{
public:

  /// @brief Constructor.
  packetizer_base(handler_base& h)
    : handler_(h)
  {}

  /// @brief Can delete through this base class.
  virtual ~packetizer_base() {}

  /// @brief Write ack packets.
  virtual void write_ack(const ack&) = 0;

  /// @brief Read ack packets.
  virtual ack read_ack(const char*) = 0;

  /// @brief Write repair packets.
  virtual void write_repair(const repair&) = 0;

  /// @brief Read repair packets.
  virtual repair read_repair(const char*) = 0;

  /// @brief Write source packets.
  virtual void write_source(const source&) = 0;

  /// @brief Read source packets.
  virtual source read_source(const char*) = 0;

  /// @brief Convenient method to write data using user's handler.
  void
  write(std::size_t len, const void* data)
  noexcept
  {
    handler_.on_ready_data(len, reinterpret_cast<const char*>(data));
  }

private:

  /// @brief The handler which serializes packets.
  handler_base& handler_;
};

/*------------------------------------------------------------------------------------------------*/

}} // namespace ntc::detail
