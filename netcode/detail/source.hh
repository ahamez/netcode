#pragma once

#include "netcode/detail/symbol_buffer.hh"
#include "netcode/types.hh"

namespace ntc { namespace detail {

/*------------------------------------------------------------------------------------------------*/

/// @internal
/// @brief A source packet holding a user's symbol.
class source final
{
public:

  source(std::uint32_t id, symbol_buffer&& buffer)
    : id_{id}, symbol_buffer_{std::move(buffer)}
  {}

  std::uint32_t
  id()
  const noexcept
  {
    return id_;
  }

  const symbol_buffer&
  buffer()
  const noexcept
  {
    return symbol_buffer_;
  }

private:

  std::uint32_t id_;
  symbol_buffer symbol_buffer_;
};

/*------------------------------------------------------------------------------------------------*/

}} // namespace ntc::detail
