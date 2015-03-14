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

  source(id_type id, symbol_buffer&& buffer)
    : id_{id}, symbol_buffer_{std::move(buffer)}
  {}

  id_type
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

  id_type id_;
  symbol_buffer symbol_buffer_;
};

/*------------------------------------------------------------------------------------------------*/

}} // namespace ntc::detail
