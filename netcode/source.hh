#pragma once

#include <arpa/inet.h> // htonl

#include "netcode/types.hh"

namespace ntc {

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

  /// @internal
  void
  write(const std::function<write_fn>& writer)
  const noexcept
  {
    const auto network_id = htonl(id_);
    writer(sizeof(id_type), reinterpret_cast<const char*>(&network_id));
    std::uint16_t sz = symbol_buffer_.size();
    sz = htons(sz);
    writer(sizeof(std::uint16_t), reinterpret_cast<const char*>(&sz));
    writer(symbol_buffer_.size(), symbol_buffer_.data());
  }

private:

  id_type id_;
  symbol_buffer_type symbol_buffer_;
};

/*------------------------------------------------------------------------------------------------*/

} // namespace ntc
