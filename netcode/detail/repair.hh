#pragma once

#include <vector>

#include "netcode/detail/symbol_buffer.hh"
#include "netcode/types.hh"

namespace ntc { namespace detail {

/*------------------------------------------------------------------------------------------------*/

/// @internal
/// @brief A repair packet.
class repair final
{
public:

  repair(id_type id)
    : id_{id}
    , sources_ids_{}
    , buffer_{}
  {}

  id_type
  id()
  const noexcept
  {
    return id_;
  }

  id_type&
  id()
  noexcept
  {
    return id_;
  }

  std::vector<id_type>&
  sources()
  noexcept
  {
    return sources_ids_;
  }

  detail::symbol_buffer&
  buffer()
  noexcept
  {
    return buffer_;
  }

  void
  reset()
  noexcept
  {
    sources_ids_.resize(0);
    buffer_.resize(0);
  }

private:

  id_type id_;
  std::vector<id_type> sources_ids_;
  detail::symbol_buffer buffer_;
};

/*------------------------------------------------------------------------------------------------*/

}} // namespace ntc::detail
