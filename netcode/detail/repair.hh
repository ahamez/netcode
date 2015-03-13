#pragma once

#include <arpa/inet.h> // htonl
#include <vector>

#include "netcode/detail/types.hh"

namespace ntc { namespace detail {

/*------------------------------------------------------------------------------------------------*/

/// @internal
class repair
{
public:

  repair(id_type id)
    : id_{id}
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
    return sources_;
  }

private:

  id_type id_;
  std::vector<id_type> sources_;
  detail::symbol_buffer_type symbol_;
};

/*------------------------------------------------------------------------------------------------*/

}} // namespace ntc::detail
