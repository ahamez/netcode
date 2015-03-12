#pragma once

#include <arpa/inet.h> // htonl

#include "netcode/types.hh"
#include "netcode/detail/types.hh"

namespace ntc { namespace detail {

/*------------------------------------------------------------------------------------------------*/

/// @internal
class source
{
public:

  source(id_type id, symbol_buffer_type&& buffer)
    : id_{id}, symbol_buffer_{std::move(buffer)}
  {}

  id_type
  id()
  const noexcept
  {
    return id_;
  }

  void
  write(const std::function<writer_fn>& writer)
  const noexcept
  {
    static const auto packet_ty = static_cast<std::uint8_t>(packet_type::source);
    const auto network_id = htonl(id_);
    const auto sz = htons(static_cast<std::uint16_t>(symbol_buffer_.size()));

    writer(sizeof(std::uint8_t) , reinterpret_cast<const char*>(&packet_ty));
    writer(sizeof(id_type)      , reinterpret_cast<const char*>(&network_id));
    writer(sizeof(std::uint16_t), reinterpret_cast<const char*>(&sz));
    writer(symbol_buffer_.size(), symbol_buffer_.data());
  }

private:

  id_type id_;
  symbol_buffer_type symbol_buffer_;
};

/*------------------------------------------------------------------------------------------------*/

}} // namespace ntc::detail
