#pragma once

#include <arpa/inet.h> // htonl, htons

#include "netcode/detail/serializer.hh"
#include "netcode/detail/types.hh"

namespace ntc { namespace detail { namespace protocol {

/*------------------------------------------------------------------------------------------------*/

/// @internal
/// @brief A simple protocol with no optimization whatsoever.
struct simple final
  : public serializer
{
  // Inherit constructors.
  using serializer::serializer;

  void
  write_ack(const ack& packet)
  override
  {
    static const auto packet_ty = static_cast<std::uint8_t>(packet_type::ack);
    handler().on_ready_packet( sizeof(std::uint8_t)
                             , reinterpret_cast<const char*>(&packet_ty));
  }

  ack
  read_ack(const char*)
  override
  {
    return {};
  }

  void
  write_repair(const repair& pkt)
  override
  {
    static const auto packet_ty = static_cast<std::uint8_t>(packet_type::repair);
    const auto network_id = htonl(pkt.id());

    handler().on_ready_packet( sizeof(std::uint8_t)
                             , reinterpret_cast<const char*>(&packet_ty));
    handler().on_ready_packet( sizeof(id_type)
                             , reinterpret_cast<const char*>(&network_id));
  }

  repair
  read_repair(const char*)
  override
  {
    return {0};
  }

  void
  write_source(const source& pkt)
  override
  {
    static const auto packet_ty = static_cast<std::uint8_t>(packet_type::source);
    const auto network_id = htonl(pkt.id());
    const auto sz = htons(static_cast<std::uint16_t>(pkt.symbol_buffer().size()));

    handler().on_ready_packet( sizeof(std::uint8_t)
                             , reinterpret_cast<const char*>(&packet_ty));
    handler().on_ready_packet( sizeof(id_type)
                             , reinterpret_cast<const char*>(&network_id));
    handler().on_ready_packet( sizeof(std::uint16_t)
                             , reinterpret_cast<const char*>(&sz));
    handler().on_ready_packet( pkt.symbol_buffer().size()
                             , pkt.symbol_buffer().data());
  }

  source
  read_source(const char*)
  override
  {
    symbol_buffer_type buffer;
    return {0, std::move(buffer)};
  }
};

/*------------------------------------------------------------------------------------------------*/

}}} // namespace ntc::detail::protocol
