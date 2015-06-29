#pragma once

#include <algorithm> // copy_n
#include <cassert>
#include <iterator>  // back_inserter
#include <limits>
#include <numeric>   // adjacent_difference, partial_sum
#include <utility>   // declval, pair
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
    // Write packet type.
    static const auto packet_ty = static_cast<std::uint8_t>(packet_type::ack);
    write<std::uint8_t>(packet_ty);

    // Write the number of packets received since last ack.
    write<std::uint16_t>(a.nb_packets());

    // Write source identifiers.
    write(a.source_ids());

    // End of data.
    mark_end();
  }

  /// @throw overflow_error
  std::pair<ack, std::size_t>
  read_ack(packet&& p)
  {
    // Packet type should have been verified by the caller.
    assert(get_packet_type(p) == packet_type::ack);

    const char* data = p.data();
    // To prevent overrun
    auto max_len = p.size();

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
  write_repair(const encoder_repair& r)
  {
    assert(r.symbol().size() > 0 && "A repair's symbol shall not be empty");

    // Write packet type.
    static const auto packet_ty = static_cast<std::uint8_t>(packet_type::repair);
    write<std::uint8_t>(packet_ty);

    // Write packet identifier.
    write<std::uint32_t>(r.id());

    // Write size of the repair symbol.
    write<std::uint16_t>(r.symbol().size());

    // Write repair symbol.
    write(r.symbol().data(), r.symbol().size());

    // Write source identifiers.
    write(r.source_ids());

    // Write encoded size.
    write<std::uint16_t>(r.encoded_size());

    // Write size of the repair symbol.
    write<std::uint16_t>(r.symbol().size());

    // Write repair symbol.
    write(r.symbol().data(), r.symbol().size());

    // End of data.
    mark_end();
  }

  /// @throw overflow_error
  std::pair<repair, std::size_t>
  read_repair(packet&& p)
  {
    // Packet type should have been verified by the caller.
    assert(get_packet_type(p) == packet_type::repair);

    const char* data = p.data();
    // To prevent overrun
    auto max_len = p.size();

    // Keep the initial memory location.
    const auto begin = reinterpret_cast<std::size_t>(data);

    // Skip packet type.
    read<std::uint8_t>(data, max_len);

    // Read identifier.
    const auto id = read<std::uint32_t>(data, max_len);

    // Read size of the repair symbol.
    const auto symbol_size = read<std::uint16_t>(data, max_len);

    // Skip the repair symbol.
    data += symbol_size;

    // Read source identifiers
    auto ids = read_ids(data, max_len);

    // Read encoded size.
    const auto encoded_sz = read<std::uint16_t>(data, max_len);

    return std::make_pair( repair{id, encoded_sz, std::move(ids), std::move(p), symbol_size}
                         , reinterpret_cast<std::size_t>(data) - begin); // Number of read bytes.
  }

  void
  write_source(const encoder_source& src)
  {
    // Write packet type.
    static const auto packet_ty = static_cast<std::uint8_t>(packet_type::source);
    write<std::uint8_t>(packet_ty);

    // Write source identifier.
    write<std::uint32_t>(src.id());

    // Write user size of the repair symbol.
    write<std::uint16_t>(src.symbol().size());

    // Write source symbol.
    write(src.symbol().data(), src.symbol().size());

    // End of data.
    mark_end();
  }

  /// @throw overflow_error
  std::pair<source, std::size_t>
  read_source(packet&& p)
  {
    // Packet type should have been verified by the caller.
    assert(get_packet_type(p) == packet_type::source);

    const char* data = p.data();
    // To prevent overrun
    auto max_len = p.size();

    // Keep the initial memory location.
    const auto begin = reinterpret_cast<std::size_t>(data);

    // Skip packet type.
    read<std::uint8_t>(data, max_len);

    // Read identifier.
    const auto id = read<std::uint32_t>(data, max_len);

    // Read user size of the source symbol.
    const auto symbol_size = read<std::uint16_t>(data, max_len);

    return std::make_pair( source{id, std::move(p), symbol_size}
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
    if (max_len < sizeof(T))
    {
      throw overflow_error{};
    }
    // A temporary placeholder is needed to have the correct alignment for type T.
    T tmp;
    std::copy_n(data, sizeof(T), reinterpret_cast<char*>(&tmp));
    const auto res = boost::endian::big_to_native(tmp);
    data += sizeof(T);
    max_len -= sizeof(T);
    return res;
  }

  /// @brief Convenient method to write data using user's handler.
  void
  write(const char* data, std::size_t len)
  noexcept(noexcept(std::declval<PacketHandler>()(nullptr, 0ul)))
  {
    m_packet_handler(reinterpret_cast<const char*>(data), len);
  }

  /// @brief Convenient method to write data using user's handler.
  template <typename T, typename U>
  void
  write(const U& data)
  noexcept(noexcept(std::declval<PacketHandler>()(nullptr, 0ul)))
  {
    const auto native = boost::endian::native_to_big(static_cast<T>(data));
    m_packet_handler(reinterpret_cast<const char*>(&native), sizeof(T));
  }

  /// @brief Serialize a list of source identifiers.
  ///
  /// The list is compressed using a RLE encoding on adjacent differences.
  void
  write(const source_id_list& ids)
  {
    m_difference_buffer.clear();
    m_rle_buffer.clear();

    if (ids.size() == 0)
    {
      static constexpr std::uint16_t zero = 0;
      write<std::uint16_t>(zero);
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
      m_rle_buffer.emplace_back(run_length, *cit);
      ++cit;
    }

    // Write the number of elements (number of pairs + the first identifier).
    write<std::uint16_t>(m_rle_buffer.size() + 1);

    // Write first identifier.
    write<std::uint32_t>(*ids.begin());

    for (const auto& pair : m_rle_buffer)
    {
      write<std::uint8_t>(pair.first);
      write<std::uint16_t>(pair.second);
    }
  }

  /// @brief Deserialize a list of source identifiers.
  source_id_list
  read_ids(const char*& data, std::size_t& max_len)
  {
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
