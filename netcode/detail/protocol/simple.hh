#pragma once

#include <arpa/inet.h> // htonl

#include "netcode/detail/serializer.hh"
#include "netcode/detail/types.hh"

namespace ntc { namespace detail { namespace protocol {

/*------------------------------------------------------------------------------------------------*/

/// @internal
struct simple
  : public serializer
{
  void
  operator()(const ack& packet, const std::function<on_ready_packet_fn>& handler)
  const override
  {
    static const auto packet_ty = static_cast<std::uint8_t>(packet_type::ack);
    handler(sizeof(std::uint8_t), reinterpret_cast<const char*>(&packet_ty));
  }

  void
  operator()(const repair& pkt, const std::function<on_ready_packet_fn>& handler)
  const override
  {
    static const auto packet_ty = static_cast<std::uint8_t>(packet_type::repair);
    const auto network_id = htonl(pkt.id());

    handler(sizeof(std::uint8_t), reinterpret_cast<const char*>(&packet_ty));
    handler(sizeof(id_type)     , reinterpret_cast<const char*>(&network_id));
  }

  void
  operator()(const source& pkt, const std::function<on_ready_packet_fn>& handler)
  const override
  {
    static const auto packet_ty = static_cast<std::uint8_t>(packet_type::source);
    const auto network_id = htonl(pkt.id());
    const auto sz = htons(static_cast<std::uint16_t>(pkt.symbol_buffer().size()));

    handler(sizeof(std::uint8_t)      , reinterpret_cast<const char*>(&packet_ty));
    handler(sizeof(id_type)           , reinterpret_cast<const char*>(&network_id));
    handler(sizeof(std::uint16_t)     , reinterpret_cast<const char*>(&sz));
    handler(pkt.symbol_buffer().size(), pkt.symbol_buffer().data());
  }
};

/*------------------------------------------------------------------------------------------------*/

}}} // namespace ntc::detail::protocol
