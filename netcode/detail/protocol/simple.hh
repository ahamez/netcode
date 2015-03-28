#pragma once

#include <arpa/inet.h> // htonl, htons
#include <cassert>
#include <iterator>    // back_inserter

#include "netcode/detail/buffer.hh"
#include "netcode/detail/multiple.hh"
#include "netcode/detail/packet_type.hh"
#include "netcode/detail/serializer.hh"
#include "netcode/detail/source_id_list.hh"

namespace ntc { namespace detail { namespace protocol {

/*------------------------------------------------------------------------------------------------*/

/// @internal
/// @brief A simple protocol with no optimization whatsoever.
struct simple final
  : public serializer_base
{
  /// @brief Constructor. Forward to base clase constructor.
  ///
  /// @note @code using serializer::serializer @endcode would be the right way to write it,
  /// but GCC 4.7 doesn't recognize this feature.
  template <typename... Args>
  simple(Args&&... args)
    : serializer_base{std::forward<Args>(args)...}
  {}

  void
  write_ack(const ack& pkt)
  override
  {
    // Prepare packet type.
    static const auto packet_ty = static_cast<std::uint8_t>(packet_type::ack);

    // Prepare number of source identifiers.
    const auto network_sz = htons(static_cast<std::uint16_t>(pkt.source_ids().size()));

    // Write packet type.
    write(sizeof(std::uint8_t), &packet_ty);

    // Write number of source identifiers.
    write(sizeof(std::uint16_t), &network_sz);

    // Write source identifiers.
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
    assert(get_packet_type(data) == packet_type::ack);

    // Skip packet type.
    data += sizeof(std::uint8_t);

    // Read size.
    const std::uint16_t nb_ids = ntohs(*reinterpret_cast<const std::uint16_t*>(data));
    data += sizeof(std::uint16_t);

    // Read source ids.
    source_id_list ids;
    ids.reserve(nb_ids);
    for (auto i = 0ul; i < nb_ids; ++i)
    {
      ids.insert(ids.end(), ntohl(*reinterpret_cast<const std::uint32_t*>(data)));
      data += sizeof(std::uint32_t);
    }

    return {std::move(ids)};
  }

  void
  write_repair(const repair& pkt)
  override
  {
    // Prepare packet type.
    static const auto packet_ty = static_cast<std::uint8_t>(packet_type::repair);

    // Prepare packet identifier.
    const auto network_id = htonl(pkt.id());

    // Prepare number of source identifiers.
    const auto network_nb_ids = htons(static_cast<std::uint16_t>(pkt.source_ids().size()));

    // Prepare repair symbol size.
    const auto network_sz = htons(static_cast<std::uint16_t>(pkt.buffer().size()));

    // Packet type.
    write(sizeof(std::uint8_t), &packet_ty);

    // Write packet identifier.
    write(sizeof(std::uint32_t), &network_id);

    // Write number of source identifiers.
    write(sizeof(std::uint16_t), &network_nb_ids);

    // Write source identifiers.
    for (const auto id : pkt.source_ids())
    {
      const std::uint32_t network_id = htonl(id);
      write(sizeof(std::uint32_t), &network_id);
    }

    // Write size of the repair symbol.
    write(sizeof(std::uint16_t), &network_sz);

    // Write repair symbol.
    write(pkt.buffer().size(), pkt.buffer().data());
  }

  repair
  read_repair(const char* data)
  override
  {
    // Packet type should have been verified by the caller.
    assert(get_packet_type(data) == packet_type::repair);

    // Skip packet type.
    data += sizeof(std::uint8_t);

    // Read identifier.
    const auto id = ntohl(*reinterpret_cast<const std::uint32_t*>(data));
    data += sizeof(std::uint32_t);

    // Read number of source identifiers
    const auto nb_ids = ntohs(*reinterpret_cast<const std::uint16_t*>(data));
    data += sizeof(std::uint16_t);

    // Read source ids.
    source_id_list ids;
    ids.reserve(nb_ids);
    for (auto i = 0ul; i < nb_ids; ++i)
    {
      ids.insert(ids.end(), ntohl(*reinterpret_cast<const std::uint32_t*>(data)));
      data += sizeof(std::uint32_t);
    }

    // Read size of the repair symbol.
    const auto sz = ntohs(*reinterpret_cast<const std::uint16_t*>(data));
    data += sizeof(std::uint16_t);

    // Read the repair symbol.
    zero_byte_buffer buffer;
    buffer.reserve(make_multiple(sz, 16));
    std::copy_n(data, sz, std::back_inserter(buffer));

    return {id, std::move(ids), std::move(buffer)};
  }

  void
  write_source(const source& pkt)
  override
  {
    // Prepare packet type.
    static const auto packet_ty = static_cast<std::uint8_t>(packet_type::source);

    // Prepare packet identifier.
    const auto network_id = htonl(pkt.id());

    // Prepare real source symbol size.
    const auto network_sz = htons(static_cast<std::uint16_t>(pkt.buffer().size()));

    // Prepare user symbol size.
    const auto network_user_sz = htons(static_cast<std::uint16_t>(pkt.user_size()));

    // Write packet type.
    write(sizeof(std::uint8_t), &packet_ty);

    // Write source identifier.
    write(sizeof(std::uint32_t), &network_id);

    // Write real source symbol size.
    write(sizeof(std::uint16_t), &network_sz);

    // Write user size of the repair symbol.
    write(sizeof(std::uint16_t), &network_user_sz);

    // Write source symbol.
    write(pkt.user_size(), pkt.buffer().data());
  }

  source
  read_source(const char* data)
  override
  {
    // Packet type should have been verified by the caller.
    assert(get_packet_type(data) == packet_type::source);

    // Skip packet type.
    data += sizeof(std::uint8_t);

    // Read identifier.
    const auto id = ntohl(*reinterpret_cast<const std::uint32_t*>(data));
    data += sizeof(std::uint32_t);

    // Read real size of the source symbol.
    const auto sz = ntohs(*reinterpret_cast<const std::uint16_t*>(data));
    data += sizeof(std::uint16_t);

    // Read user size of the source symbol.
    const auto user_sz = ntohs(*reinterpret_cast<const std::uint16_t*>(data));
    data += sizeof(std::uint16_t);

    // Read the source symbol.
    byte_buffer buffer;
    buffer.reserve(make_multiple(sz, 16));
    std::copy_n(data, sz, std::back_inserter(buffer));

    return {id, std::move(buffer), user_sz};
  }
};

/*------------------------------------------------------------------------------------------------*/

}}} // namespace ntc::detail::protocol
