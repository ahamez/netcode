#pragma once

#include <cassert>
#include <iterator> // back_inserter
#include <limits>
#include <numeric>  // adjacent_difference, partial_sum
#include <utility>  // declval, pair
#include <vector>

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wconversion"
#include <boost/endian/conversion.hpp>
#pragma GCC diagnostic pop

#include "netcode/detail/ack.hh"
#include "netcode/detail/buffer.hh"
#include "netcode/detail/packet_type.hh"
#include "netcode/detail/source.hh"
#include "netcode/detail/source_id_list.hh"
#include "netcode/detail/repair.hh"
#include "netcode/errors.hh"

namespace ntc { namespace detail {

/*------------------------------------------------------------------------------------------------*/

/// @internal
/// @brief Prepare and construct ack/repair/source for network.
template <typename PacketHandler>
class packetizer final
{
public:

  /// @brief Constructor.
  explicit packetizer(PacketHandler& h)
    : m_packet_handler(h)
    , m_difference_buffer(32)
    , m_rle_buffer(32)
  {}

  void
  write_ack(const ack& a)
  {
    using namespace boost::endian;

    // Prepare packet type.
    static const auto packet_ty = static_cast<std::uint8_t>(packet_type::ack);

    // Prepare number of received packets.
    const auto network_nb_packets = native_to_big(static_cast<std::uint16_t>(a.nb_packets()));

    // Write packet type.
    write(&packet_ty, sizeof(std::uint8_t));

    // Write the number of packets received since last ack.
    write(&network_nb_packets, sizeof(std::uint16_t));

    // Write source identifiers.
    write(a.source_ids());

    // End of data.
    mark_end();
  }

  /// @throw overflow_error
  std::pair<ack, std::size_t>
  read_ack(const char* data, std::size_t max_len)
  {
    // Packet type should have been verified by the caller.
    assert(get_packet_type(data) == packet_type::ack);

    // Keep the initial memory location.
    const auto begin = reinterpret_cast<std::size_t>(data);

    // Skip packet type
    read<std::uint8_t>(data, max_len);

    // Read the number of packets received since last ack.
    const auto nb_packets = read<std::uint16_t>(data, max_len);

    // Read source identifiers
    auto ids = read_ids(data, max_len);

    return std::make_pair( ack{std::move(ids), nb_packets}
                         , reinterpret_cast<std::size_t>(data) - begin); // Number of read bytes.
  }

  void
  write_repair(const repair& r)
  {
    using namespace boost::endian;

    // Prepare packet type.
    static const auto packet_ty = static_cast<std::uint8_t>(packet_type::repair);

    // Prepare packet identifier.
    const auto network_id = native_to_big(r.id());

    // Prepare encoded size.
    const auto network_encoded_sz = native_to_big(static_cast<std::uint16_t>(r.encoded_size()));

    // Prepare repair symbol size.
    const auto network_sz = native_to_big(static_cast<std::uint16_t>(r.buffer().size()));

    // Packet type.
    write(&packet_ty, sizeof(std::uint8_t));

    // Write packet identifier.
    write(&network_id, sizeof(std::uint32_t));

    // Write source identifiers.
    write(r.source_ids());

    // Write encoded size.
    write(&network_encoded_sz, sizeof(std::uint16_t));

    // Write size of the repair symbol.
    write(&network_sz, sizeof(std::uint16_t));

    // Write repair symbol.
    write(r.buffer().data(), r.buffer().size());

    // End of data.
    mark_end();
  }

  /// @throw overflow_error
  std::pair<repair, std::size_t>
  read_repair(const char* data, std::size_t max_len)
  {
    // Packet type should have been verified by the caller.
    assert(get_packet_type(data) == packet_type::repair);

    // Keep the initial memory location.
    const auto begin = reinterpret_cast<std::size_t>(data);

    // Skip packet type.
    read<std::uint8_t>(data, max_len);

    // Read identifier.
    const auto id = read<std::uint32_t>(data, max_len);

    // Read source identifiers
    auto ids = read_ids(data, max_len);

    // Read encoded size.
    const auto encoded_sz = read<std::uint16_t>(data, max_len);

    // Read size of the repair symbol.
    const auto sz = read<std::uint16_t>(data, max_len);

    // Read the repair symbol.
    zero_byte_buffer buffer;
    buffer.reserve(sz);
    std::copy_n(data, sz, std::back_inserter(buffer));

    return std::make_pair( repair{id, encoded_sz, std::move(ids), std::move(buffer)}
                         , reinterpret_cast<std::size_t>(data) - begin); // Number of read bytes.
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

  /// @throw overflow_error
  std::pair<source, std::size_t>
  read_source(const char* data, std::size_t max_len)
  {
    // Packet type should have been verified by the caller.
    assert(get_packet_type(data) == packet_type::source);

    // Keep the initial memory location.
    const auto begin = reinterpret_cast<std::size_t>(data);

    // Skip packet type.
    read<std::uint8_t>(data, max_len);

    // Read identifier.
    const auto id = read<std::uint32_t>(data, max_len);

    // Read user size of the source symbol.
    const auto user_sz = read<std::uint16_t>(data, max_len);

    // Read the source symbol.
    byte_buffer buffer;
    buffer.reserve(user_sz);
    std::copy_n(data, user_sz, std::back_inserter(buffer));

    return std::make_pair( source{id, std::move(buffer), user_sz}
                         , reinterpret_cast<std::size_t>(data) - begin); // Number of read bytes.
  }

private:

  /// @brief Convenient method to read data and verify the size of read data.
  /// @throw overflow_error
  template <typename T>
  static
  T
  read(const char*& data, std::size_t& max_len)
  {
    using namespace boost::endian;
    if (max_len < sizeof(T))
    {
      throw overflow_error{};
    }
    const auto res = big_to_native(*reinterpret_cast<const T*>(data));
    data += sizeof(T);
    max_len -= sizeof(T);
    return res;
  }

  /// @brief Convenient method to write data using user's handler.
  void
  write(const void* data, std::size_t len)
  noexcept(noexcept(std::declval<PacketHandler>()(nullptr, 0ul)))
  {
    m_packet_handler(reinterpret_cast<const char*>(data), len);
  }

  /// @brief Serialize a list of source identifiers.
  ///
  /// The list is compressed using a RLE encoding on adjacent differences.
  void
  write(const source_id_list& ids)
  {
    using namespace boost::endian;

    m_difference_buffer.clear();
    m_rle_buffer.clear();

    if (ids.size() == 0)
    {
      static constexpr std::uint16_t zero = 0;
      write(&zero, sizeof(std::uint16_t));
      return;
    }

    // Compute adjacent differences.
    std::adjacent_difference(ids.begin(), ids.end(), std::back_inserter(m_difference_buffer));

    // Skip first id as we need to keep it on 32 bits and as its running length will always be 1.
    auto cit = std::next(m_difference_buffer.begin());
    const auto end = m_difference_buffer.end();

    // Running length encoding.
    while (cit != end)
    {
      std::uint8_t run_length = 1;
      while ( std::next(cit) != end and *cit == *std::next(cit)
              // We limit a run length to make it fit in 8 bits.
              and run_length < std::numeric_limits<std::uint8_t>::max())
      {
        ++run_length;
        ++cit;
      }
      // The cast to 16 bits is important: differences are always small and rle_buffer only
      // stores 16 bits differences.
      m_rle_buffer.emplace_back(run_length, native_to_big(static_cast<std::uint16_t>(*cit)));
      ++cit;
    }

    // Write the number of elements (number of pairs + the first identifier).
    const auto nb_elements = native_to_big(static_cast<std::uint16_t>(m_rle_buffer.size() + 1));
    write(&nb_elements, sizeof(std::uint16_t));

    // Write first identifier.
    const auto first_id = native_to_big(*ids.begin());
    write(&first_id, sizeof(std::uint32_t));

    for (const auto& pair : m_rle_buffer)
    {
      write(&pair.first, sizeof(std::uint8_t));
      write(&pair.second, sizeof(std::uint16_t));
    }
  }

  /// @brief Deserialize a list of source identifiers.
  source_id_list
  read_ids(const char*& data, std::size_t& max_len)
  {
    using namespace boost::endian;

    source_id_list ids;

    m_difference_buffer.clear();
    m_rle_buffer.clear();

    const auto nb_elements = read<std::uint16_t>(data, max_len);
    if (nb_elements == 0)
    {
      return ids;
    }

    // Read first identifier.
    const auto first_id = read<std::uint32_t>(data, max_len);
    m_difference_buffer.push_back(first_id);

    // Reverse running length encoding on the fly.
    const auto nb_pairs = nb_elements - 1u; // Remove the first identifier.
    for (auto i = 0ul; i < nb_pairs; ++i)
    {
      const auto run_length = read<std::uint8_t>(data, max_len);
      const auto value = read<std::uint16_t>(data, max_len);

      for (auto j = 0u; j < run_length; ++j)
      {
        m_difference_buffer.push_back(value);
      }
    }

    // Revert to list of source identifiers.
    std::partial_sum( m_difference_buffer.begin(), m_difference_buffer.end()
                    , std::inserter(ids, ids.end()));

    return ids;
  }

  /// @brief Convenient method to indicate end of data to user's handler.
  void
  mark_end()
  noexcept(noexcept(std::declval<PacketHandler>()()))
  {
    m_packet_handler();
  }

private:

  /// @brief The handler which serializes packets.
  PacketHandler& m_packet_handler;

  /// @brief A pre-allocated buffer to re-use when computing adjacent difference for ids list.
  /// @note We use a 32-bits type as the first element will always be exactly the same as the
  /// ids list, which are on 32 bits.
  std::vector<std::uint32_t> m_difference_buffer;

  /// @brief A pre-allocated buffer to re-use when performing the running length encoding.
  std::vector<std::pair<std::uint8_t, std::uint16_t>> m_rle_buffer;
};

/*------------------------------------------------------------------------------------------------*/

}} // namespace ntc::detail
