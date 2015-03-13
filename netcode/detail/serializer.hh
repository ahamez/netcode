#pragma once

#include "netcode/detail/ack.hh"
#include "netcode/detail/handler.hh"
#include "netcode/detail/repair.hh"
#include "netcode/detail/source.hh"
#include "netcode/detail/types.hh"

namespace ntc { namespace detail {

/*------------------------------------------------------------------------------------------------*/

/// @internal
class serializer
{
public:

  /// @brief Constructor.
  serializer(handler_base& h)
    : handler_{h}
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

protected:

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
