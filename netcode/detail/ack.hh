#pragma once

#include "netcode/detail/source_id_list.hh"

namespace ntc { namespace detail {

/*------------------------------------------------------------------------------------------------*/

/// @internal
/// @brief An acknowledgement packet.
class ack final
{
public:

  /// @brief Can't copy-construct an ack.
  ack(const ack&) = delete;

  /// @brief Can't copy an ack.
  ack& operator=(const ack&) = delete;

  /// @brief Can move-construct an ack.
  ack(ack&&) = default;

  /// @brief Can move an ack.
  ack& operator=(ack&&) = default;

  /// @brief Constructor
  ack()
    : source_ids_{}, nb_packets_{0}
  {}

  /// @brief Constructor.
  explicit ack(source_id_list&& source_ids, std::uint16_t nb_packets)
    : source_ids_{std::move(source_ids)}
    , nb_packets_{nb_packets}
  {}

  /// @brief Get the list of acknowledged sources.
  const source_id_list&
  source_ids()
  const noexcept
  {
    return source_ids_;
  }

  /// @brief Get the list of acknowledged sources.
  source_id_list&
  source_ids()
  noexcept
  {
    return source_ids_;
  }

  /// @brief Reset this ack.
  ///
  /// List of source identifiers is resized to 0.
  void
  reset()
  noexcept
  {
    source_ids_.clear();
    nb_packets_ = 0;
  }

  /// @brief Get the number of packets received by the decoder since the last ack.
  std::uint16_t
  nb_packets()
  const noexcept
  {
    return nb_packets_;
  }

  /// @brief Get the number of packets received by the decoder since the last ack.
  std::uint16_t&
  nb_packets()
  noexcept
  {
    return nb_packets_;
  }

private:

  /// @brief The list of acknowledged sources.
  source_id_list source_ids_;

  /// @brief The number of received packet since the last ack.
  std::uint16_t nb_packets_;
};

/*------------------------------------------------------------------------------------------------*/

}} // namespace ntc::detail
