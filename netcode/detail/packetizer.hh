#pragma once

#include <cassert>
#include <iterator> // back_inserter
#include <utility>  // pair

#include <boost/endian/conversion.hpp>

#include "netcode/detail/ack.hh"
#include "netcode/detail/buffer.hh"
#include "netcode/detail/packet_type.hh"
#include "netcode/detail/source.hh"
#include "netcode/detail/source_id_list.hh"
#include "netcode/detail/repair.hh"

namespace ntc { namespace detail {

/*------------------------------------------------------------------------------------------------*/

/// @internal
/// @brief Prepare and construct ack/repair/source for network.
template <typename PacketHandler>
class packetizer final
{
public:

  /// @brief Constructor.
  packetizer(PacketHandler& h)
    : packet_handler_(h)
  {}

  void
  write_ack(const ack& pkt)
  {
    using namespace boost::endian;

    // Prepare packet type.
    static const auto packet_ty = static_cast<std::uint8_t>(packet_type::ack);

    // Prepare number of source identifiers.
    const auto network_sz = native_to_big(static_cast<std::uint16_t>(pkt.source_ids().size()));

    // Write packet type.
    write(&packet_ty, sizeof(std::uint8_t));

    // Write number of source identifiers.
    write(&network_sz, sizeof(std::uint16_t));

    // Write source identifiers.
    for (const auto id : pkt.source_ids())
    {
      const auto network_id = native_to_big(id);
      write(&network_id, sizeof(std::uint32_t));
    }

    // End of data.
    mark_end();
  }

  std::pair<ack, std::size_t>
  read_ack(const char* data)
  {
    using namespace boost::endian;

    // Packet type should have been verified by the caller.
    assert(get_packet_type(data) == packet_type::ack);

    // Keep the initial memory location.
    const auto begin = reinterpret_cast<std::size_t>(data);

    // Skip packet type.
    data += sizeof(std::uint8_t);

    // Read size.
    const auto nb_ids = big_to_native(*reinterpret_cast<const std::uint16_t*>(data));
    data += sizeof(std::uint16_t);

    // Read source ids.
    source_id_list ids;
    ids.reserve(nb_ids);
    for (auto i = 0ul; i < nb_ids; ++i)
    {
      ids.insert(ids.end(), big_to_native(*reinterpret_cast<const std::uint32_t*>(data)));
      data += sizeof(std::uint32_t);
    }

    return std::make_pair( ack{std::move(ids)}
                         , reinterpret_cast<std::size_t>(data) - begin);
  }

  void
  write_repair(const repair& pkt)
  {
    using namespace boost::endian;

    // Prepare packet type.
    static const auto packet_ty = static_cast<std::uint8_t>(packet_type::repair);

    // Prepare packet identifier.
    const auto network_id = native_to_big(pkt.id());

    // Prepare number of source identifiers.
    const auto network_nb_ids
      = native_to_big(static_cast<std::uint16_t>(pkt.source_ids().size()));

    // Prepare user size.
    const auto network_user_sz = native_to_big(static_cast<std::uint16_t>(pkt.size()));

    // Prepare repair symbol size.
    const auto network_sz = native_to_big(static_cast<std::uint16_t>(pkt.buffer().size()));

    // Packet type.
    write(&packet_ty, sizeof(std::uint8_t));

    // Write packet identifier.
    write(&network_id, sizeof(std::uint32_t));

    // Write number of source identifiers.
    write(&network_nb_ids, sizeof(std::uint16_t));

    // Write source identifiers.
    for (const auto id : pkt.source_ids())
    {
      const auto network_id = native_to_big(id);
      write(&network_id, sizeof(std::uint32_t));
    }

    // Write user size.
    write(&network_user_sz, sizeof(std::uint16_t));

    // Write size of the repair symbol.
    write(&network_sz, sizeof(std::uint16_t));

    // Write repair symbol.
    write(pkt.buffer().data(), pkt.buffer().size());

    // End of data.
    mark_end();
  }

  std::pair<repair, std::size_t>
  read_repair(const char* data)
  {
    using namespace boost::endian;

    // Packet type should have been verified by the caller.
    assert(get_packet_type(data) == packet_type::repair);

    // Keep the initial memory location.
    const auto begin = reinterpret_cast<std::size_t>(data);

    // Skip packet type.
    data += sizeof(std::uint8_t);

    // Read identifier.
    const auto id = big_to_native(*reinterpret_cast<const std::uint32_t*>(data));
    data += sizeof(std::uint32_t);

    // Read number of source identifiers
    const auto nb_ids = big_to_native(*reinterpret_cast<const std::uint16_t*>(data));
    data += sizeof(std::uint16_t);

    // Read source ids.
    source_id_list ids;
    ids.reserve(nb_ids);
    for (auto i = 0ul; i < nb_ids; ++i)
    {
      ids.insert(ids.end(), big_to_native(*reinterpret_cast<const std::uint32_t*>(data)));
      data += sizeof(std::uint32_t);
    }

    // Read user size.
    const auto user_sz = big_to_native(*reinterpret_cast<const std::uint16_t*>(data));
    data += sizeof(std::uint16_t);

    // Read size of the repair symbol.
    const auto sz = big_to_native(*reinterpret_cast<const std::uint16_t*>(data));
    data += sizeof(std::uint16_t);

    // Read the repair symbol.
    zero_byte_buffer buffer;
    buffer.reserve(sz);
    std::copy_n(data, sz, std::back_inserter(buffer));

    return std::make_pair( repair{id, user_sz, std::move(ids), std::move(buffer)}
                         , reinterpret_cast<std::size_t>(data) - begin);
  }

  void
  write_source(const source& src)
  {
    using namespace boost::endian;

    // Prepare packet type.
    static const auto packet_ty = static_cast<std::uint8_t>(packet_type::source);

    // Prepare packet identifier.
    const auto network_id = native_to_big(src.id());

    // Prepare user symbol size.
    const auto network_user_sz = native_to_big(static_cast<std::uint16_t>(src.user_size()));

    // Write packet type.
    write(&packet_ty, sizeof(std::uint8_t));

    // Write source identifier.
    write(&network_id, sizeof(std::uint32_t));

    // Write user size of the repair symbol.
    write(&network_user_sz, sizeof(std::uint16_t));

    // Write source symbol.
    write(src.buffer().data(), src.user_size());

    // End of data.
    mark_end();
  }

  std::pair<source, std::size_t>
  read_source(const char* data)
  {
    using namespace boost::endian;

    // Packet type should have been verified by the caller.
    assert(get_packet_type(data) == packet_type::source);

    // Keep the initial memory location.
    const auto begin = reinterpret_cast<std::size_t>(data);

    // Skip packet type.
    data += sizeof(std::uint8_t);

    // Read identifier.
    const auto id = big_to_native(*reinterpret_cast<const std::uint32_t*>(data));
    data += sizeof(std::uint32_t);

    // Read user size of the source symbol.
    const auto user_sz = big_to_native(*reinterpret_cast<const std::uint16_t*>(data));
    data += sizeof(std::uint16_t);

    // Read the source symbol.
    byte_buffer buffer;
    buffer.reserve(user_sz);
    std::copy_n(data, user_sz, std::back_inserter(buffer));

    return std::make_pair( source{id, std::move(buffer), user_sz}
                         , reinterpret_cast<std::size_t>(data) - begin);
  }

private:

  /// @brief Convenient method to write data using user's handler.
  void
  write(const void* data, std::size_t len)
  noexcept
  {
    packet_handler_(reinterpret_cast<const char*>(data), len);
  }

  /// @brief Convenient method to indicate end of data to user's handler.
  void
  mark_end()
  noexcept
  {
    packet_handler_();
  }

  /// @brief The handler which serializes packets.
  PacketHandler& packet_handler_;
};

/*------------------------------------------------------------------------------------------------*/

}} // namespace ntc::detail
