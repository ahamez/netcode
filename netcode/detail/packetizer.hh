#pragma once

#include <cassert>
#include <iterator> // back_inserter
#include <numeric>  // adjacent_difference, partial_sum
#include <utility>  // pair
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
    , difference_buffer_(32)
    , rle_buffer_(32)
  {}

  void
  write_ack(const ack& a)
  {
    using namespace boost::endian;

    // Prepare packet type.
    static const auto packet_ty = static_cast<std::uint8_t>(packet_type::ack);

    // Write packet type.
    write(&packet_ty, sizeof(std::uint8_t));

    // Write source identifiers.
    write(a.source_ids());

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

    // Read source identifiers
    source_id_list ids;
    data += read_ids(data, ids);

    return std::make_pair( ack{std::move(ids)}
                         , reinterpret_cast<std::size_t>(data) - begin);
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

    // Read source identifiers
    source_id_list ids;
    data += read_ids(data, ids);

    // Read encoded size.
    const auto encoded_sz = big_to_native(*reinterpret_cast<const std::uint16_t*>(data));
    data += sizeof(std::uint16_t);

    // Read size of the repair symbol.
    const auto sz = big_to_native(*reinterpret_cast<const std::uint16_t*>(data));
    data += sizeof(std::uint16_t);

    // Read the repair symbol.
    zero_byte_buffer buffer;
    buffer.reserve(sz);
    std::copy_n(data, sz, std::back_inserter(buffer));

    return std::make_pair( repair{id, encoded_sz, std::move(ids), std::move(buffer)}
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
  {
    packet_handler_(reinterpret_cast<const char*>(data), len);
  }

  void
  write(const source_id_list& ids)
  {
    using namespace boost::endian;

    difference_buffer_.clear();
    rle_buffer_.clear();

    // Compute adjacent differences.
    std::adjacent_difference(ids.begin(), ids.end(), std::back_inserter(difference_buffer_));

    // RLE.
    auto cit = difference_buffer_.begin();
    const auto end = difference_buffer_.end();
    while (cit != end)
    {
      std::uint16_t run_length = 1;
      while (std::next(cit) != end and *cit == *std::next(cit))
      {
        ++run_length;
        ++cit;
      }

      rle_buffer_.push_back(native_to_big(run_length));
      rle_buffer_.push_back(native_to_big(*cit));

      ++cit;
    }

    const auto sz = native_to_big(static_cast<std::uint16_t>(rle_buffer_.size()));
    write(&sz, sizeof(std::uint16_t));
    write(rle_buffer_.data(), rle_buffer_.size() * sizeof(std::uint16_t));
  }

  std::size_t
  read_ids(const char* data, source_id_list& ids)
  {
    using namespace boost::endian;

    difference_buffer_.clear();
    rle_buffer_.clear();

    const auto size = big_to_native(*reinterpret_cast<const std::uint16_t*>(data));
    data += sizeof(std::uint16_t);

    for (auto i = 0ul; i < size; ++i)
    {
      rle_buffer_.push_back(big_to_native(*reinterpret_cast<const std::uint16_t*>(data)));
      data += sizeof(std::uint16_t);
    }

    // UnRLE
    for (auto cit = rle_buffer_.begin(); cit != rle_buffer_.end();)
    {
      const auto run_length = *cit;
      const auto value = *std::next(cit);
      std::advance(cit, 2);
      for (auto j = 0u; j < run_length; ++j)
      {
        difference_buffer_.push_back(value);
      }
    }

    // Revert to list of source identifiers.
    std::partial_sum( difference_buffer_.begin(), difference_buffer_.end()
                    , std::inserter(ids, ids.end()));

    return (size + 1) * sizeof(std::uint16_t);
  }

  /// @brief Convenient method to indicate end of data to user's handler.
  void
  mark_end()
  noexcept
  {
    packet_handler_();
  }

private:

  /// @brief The handler which serializes packets.
  PacketHandler& packet_handler_;

  /// @brief
  std::vector<std::uint16_t> difference_buffer_;

  /// @brief
  std::vector<std::uint16_t> rle_buffer_;
};

/*------------------------------------------------------------------------------------------------*/

}} // namespace ntc::detail
