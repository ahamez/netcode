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

  /// @brief Default constructor.
  ack()
    : m_source_ids{}
    , m_nb_packets{0}
  {}

  /// @brief Constructor.
  explicit ack(source_id_list&& source_ids, std::uint16_t nb_packets)
    : m_source_ids{std::move(source_ids)}
    , m_nb_packets{nb_packets}
  {}

  /// @brief Get the list of acknowledged sources.
  const source_id_list&
  source_ids()
  const noexcept
  {
    return m_source_ids;
  }

  /// @brief Get the list of acknowledged sources.
  source_id_list&
  source_ids()
  noexcept
  {
    return m_source_ids;
  }

  /// @brief Reset this ack.
  ///
  /// List of source identifiers is resized to 0.
  void
  reset()
  noexcept
  {
    m_source_ids.clear();
    m_nb_packets = 0;
  }

  /// @brief Get the number of packets received by the decoder since the last ack.
  std::uint16_t
  nb_packets()
  const noexcept
  {
    return m_nb_packets;
  }

  /// @brief Get the number of packets received by the decoder since the last ack.
  std::uint16_t&
  nb_packets()
  noexcept
  {
    return m_nb_packets;
  }

private:

  /// @brief The list of acknowledged sources.
  source_id_list m_source_ids;

  /// @brief The number of received packet since the last ack.
  std::uint16_t m_nb_packets;
};

/*------------------------------------------------------------------------------------------------*/

}} // namespace ntc::detail
