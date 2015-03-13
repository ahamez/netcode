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
  write(const std::function<writer_fn>& writer)
  const noexcept
  {
    static const auto packet_ty = static_cast<std::uint8_t>(packet_type::repair);
    const auto network_id = htonl(id_);

    writer(sizeof(std::uint8_t), reinterpret_cast<const char*>(&packet_ty));
    writer(sizeof(id_type)     , reinterpret_cast<const char*>(&network_id));
  }

private:

  id_type id_;
  std::vector<id_type> sources_;
  detail::symbol_buffer_type symbol_;
};

/*------------------------------------------------------------------------------------------------*/

}} // namespace ntc::detail
