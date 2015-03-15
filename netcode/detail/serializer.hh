#pragma once

#include "netcode/detail/ack.hh"
#include "netcode/detail/handler.hh"
#include "netcode/detail/repair.hh"
#include "netcode/detail/source.hh"

namespace ntc { namespace detail {

/*------------------------------------------------------------------------------------------------*/

/// @internal
/// @brief The base class for any serialization of @ref ack, @ref repair and @ref source.
class serializer
{
public:

  /// @brief Constructor.
  serializer(handler_base& h)
    : handler_(h)
  {}

  /// @brief Can delete through this base class.
  virtual ~serializer() {}

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
    handler().on_ready_data(len, reinterpret_cast<const char*>(data));
  }

protected:

  /// @brief Provide access to the user's handler for derived classes.
  handler_base&
  handler()
  noexcept
  {
    return handler_;
  }

private:

  /// @brief The handler which will write serialized packets.
  handler_base& handler_;
};

/*------------------------------------------------------------------------------------------------*/

}} // namespace ntc::detail
