#pragma once

#include <arpa/inet.h> // htonl
#include <vector>

#include "netcode/detail/types.hh"

namespace ntc { namespace detail {

/*------------------------------------------------------------------------------------------------*/

/// @internal
class ack
{
public:

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
    static const auto packet_ty = static_cast<std::uint8_t>(packet_type::source);
    writer(sizeof(std::uint8_t), reinterpret_cast<const char*>(&packet_ty));
  }

private:

  id_type id_;
  std::vector<id_type> sources_;
};

/*------------------------------------------------------------------------------------------------*/

}} // namespace ntc::detail
