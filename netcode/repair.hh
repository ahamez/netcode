#pragma once

#include <arpa/inet.h> // htonl
#include <vector>

#include "netcode/types.hh"

namespace ntc {

/*------------------------------------------------------------------------------------------------*/

class repair
{
public:

  repair(id_type id)
    : id_{id}
  {}

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

  void
  write(const std::function<write_fn>& writer)
  const noexcept
  {
    const auto network_id = htonl(id_);
    writer(sizeof(id_type), reinterpret_cast<const char*>(&network_id));
  }

private:

  id_type id_;
  std::vector<id_type> sources_;
  symbol_buffer_type symbol_;
};

/*------------------------------------------------------------------------------------------------*/

} // namespace ntc
