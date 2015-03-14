#pragma once

#include <algorithm>   // for_each, transform
#include <arpa/inet.h> // htonl, htons
#include <cassert>

#include "netcode/detail/packet_type.hh"
#include "netcode/detail/serializer.hh"
#include "netcode/detail/symbol_buffer.hh"

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
  write_ack(const ack& pkt)
  override
  {
    static const auto packet_ty = static_cast<std::uint8_t>(packet_type::ack);
    const auto network_sz = htons(static_cast<std::uint16_t>(pkt.source_ids().size()));

    write(sizeof(std::uint8_t), &packet_ty);
    write(sizeof(std::uint16_t), &network_sz);

    for (const auto id : pkt.source_ids())
    {
      const std::uint32_t network_id = htonl(id);
      write(sizeof(std::uint32_t), &network_id);
    }
  }

  ack
  read_ack(const char* data)
  override
  {
    // Packet type should have been verified by the caller.
    assert(data[0] == static_cast<std::uint8_t>(packet_type::ack));

    // Skip packet type.
    data += sizeof(std::uint8_t);

    // Read size.
    const std::uint16_t size = ntohs(*reinterpret_cast<const std::uint16_t*>(data));
    data += sizeof(std::uint16_t);

    // Read source ids.
    const auto* data_as_ids = reinterpret_cast<const id_type*>(data);
    std::vector<id_type> ids;
    ids.reserve(size);
    std::transform( data_as_ids, data_as_ids + size, std::back_inserter(ids)
                  , [](id_type id){return ntohl(id);});

    return {std::move(ids)};
  }

  void
  write_repair(const repair& pkt)
  override
  {
    static const auto packet_ty = static_cast<std::uint8_t>(packet_type::repair);
    const auto network_id = htonl(pkt.id());
    const auto network_nb_ids = htons(static_cast<std::uint16_t>(pkt.source_ids().size()));
    const auto network_sz = htons(static_cast<std::uint16_t>(pkt.buffer().size()));

    write(sizeof(std::uint8_t), &packet_ty);
    write(sizeof(id_type), &network_id);
    write(sizeof(std::uint16_t),&network_nb_ids);
    for (const auto id : pkt.source_ids())
    {
      const std::uint32_t network_id = htonl(id);
      write(sizeof(std::uint32_t), &network_id);
    }
    write(sizeof(std::uint16_t), &network_sz);
    write(pkt.buffer().size(), pkt.buffer().data());
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
    const auto network_sz = htons(static_cast<std::uint16_t>(pkt.buffer().size()));

    write(sizeof(std::uint8_t), &packet_ty);
    write(sizeof(id_type), &network_id);
    write(sizeof(std::uint16_t), &network_sz);
    write(pkt.buffer().size(), pkt.buffer().data());
  }

  source
  read_source(const char*)
  override
  {
    symbol_buffer buffer;
    return {0, std::move(buffer)};
  }
};

/*------------------------------------------------------------------------------------------------*/

}}} // namespace ntc::detail::protocol
