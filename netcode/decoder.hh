#pragma once

#ifdef NTC_DUMP_PACKETS
#include <fstream>
#include "netcode/detail/serialize_packet.hh"
#endif

#include <chrono>

#include "netcode/detail/decoder.hh"
#include "netcode/detail/packet_type.hh"
#include "netcode/detail/packetizer.hh"
#include "netcode/detail/repair.hh"
#include "netcode/detail/source.hh"
#include "netcode/detail/visibility.hh"
#include "netcode/errors.hh"
#include "netcode/in_order.hh"

namespace ntc {

/*------------------------------------------------------------------------------------------------*/

/// @ingroup ntc_decoder
/// @brief The class to interact with on the receiver side.
template <typename PacketHandler, typename DataHandler>
class NTC_PUBLIC decoder final
{
public:

  /// @brief The type of the handler that processes data ready to be sent on the network.
  using packet_handler_type = PacketHandler;

  /// @brief The type of the handler that processes decoded or received data.
  using data_handler_type = DataHandler;

public:

  /// @brief Can't copy-construct a decoder.
  decoder(const decoder&) = delete;

  /// @brief Can't copy a decoder.
  decoder& operator=(const decoder&) = delete;

  /// @brief Can't move-construct a decoder.
  decoder(decoder&&) = delete;

  /// @brief Can't move a decoder.
  decoder& operator=(decoder&&) = delete;

  /// @brief Constructor.
  template <typename PacketHandler_, typename DataHandler_>
  decoder( std::uint8_t galois_field_size, in_order ordered, PacketHandler_&& packet_handler
         , DataHandler_&& data_handler)
    : m_galois_field_size{galois_field_size}
    , m_ack_frequency{std::chrono::milliseconds{100}}
    , m_ack_nb_packets{50}
    , m_last_ack_date(std::chrono::steady_clock::now())
    , m_ack{}
    , m_decoder{ m_galois_field_size
                 // The real decoder needs to know how to handle decoded or received sources.
               , [this](const detail::decoder_source& src){handle_source(src);}
               , ordered}
    , m_packet_handler(std::forward<PacketHandler_>(packet_handler))
    , m_data_handler(std::forward<DataHandler_>(data_handler))
    , m_packetizer{m_packet_handler}
    , m_nb_received_repairs{0}
    , m_nb_received_sources{0}
    , m_nb_sent_ack{0}
#ifdef NTC_DUMP_PACKETS
    , m_dump_file{NTC_DUMP_PACKETS_FILE}
#endif
  {
    // Let's reserve some memory for the ack, it will most likely avoid memory re-allocations.
    // Uncomment the following when the undefined behavior spotted by GCC 5.1 -fsanitize=undefined
    // is fixed. In the meantime, it's not a real problem,it will just cost a few initial
    // allocations before the source ids list grows to a suitable size.
    // m_ack.source_ids().reserve(128);
  }

  /// @brief Notify the decoder of an incoming packet
  std::size_t
  operator()(const packet& p)
  {
    return operator()(packet{p});
  }

  /// @brief Notify the decoder of an incoming packet
  std::size_t
  operator()(packet&& p)
  {
    assert(p.size() != 0 && "empty packet");

#ifdef NTC_DUMP_PACKETS
    detail::serialize_packet::write(m_dump_file, p);
#endif

    switch (detail::get_packet_type(p))
    {
      case detail::packet_type::repair:
      {
        ++m_nb_received_repairs;
        ++m_ack.nb_packets();
        auto res = m_packetizer.read_repair(std::move(p));
        m_decoder(std::move(res.first));
        return res.second;
      }

      case detail::packet_type::source:
      {
        ++m_nb_received_sources;
        ++m_ack.nb_packets();
        auto res = m_packetizer.read_source(std::move(p));
        m_decoder(std::move(res.first));
        return res.second;
      }

      default:
      {
        throw packet_type_error{p};
      }
    }
  }

  /// @brief Get the data handler.
  const packet_handler_type&
  packet_handler()
  const noexcept
  {
    return m_packet_handler;
  }

  /// @brief Get the data handler.
  packet_handler_type&
  packet_handler()
  noexcept
  {
    return m_packet_handler;
  }

  /// @brief Get the data handler.
  const data_handler_type&
  data_handler()
  const noexcept
  {
    return m_data_handler;
  }

  /// @brief Get the data handler.
  data_handler_type&
  data_handler()
  noexcept
  {
    return m_data_handler;
  }

  /// @brief Get the total number of decoded sources.
  std::size_t
  nb_decoded()
  const noexcept
  {
    return m_decoder.nb_decoded();
  }

  /// @brief Get the number of times the full decoding failed.
  std::size_t
  nb_failed_full_decodings()
  const noexcept
  {
    return m_decoder.nb_failed_full_decodings();
  }

  /// @brief Get the current number of missing sources.
  std::size_t
  nb_missing_sources()
  const noexcept
  {
    return m_decoder.missing_sources().size();
  }

  /// @brief Get the total number of received repairs.
  std::size_t
  nb_received_repairs()
  const noexcept
  {
    return m_nb_received_repairs;
  }

  /// @brief Get the total number of received sources.
  std::size_t
  nb_received_sources()
  const noexcept
  {
    return m_nb_received_sources;
  }

  /// @brief Get the number of sent ack.
  std::size_t
  nb_sent_acks()
  const noexcept
  {
    return m_nb_sent_ack;
  }

  /// @brief Get the number of repairs that were dropped because they were useless.
  std::size_t
  nb_useless_repairs()
  const noexcept
  {
    return m_decoder.nb_useless_repairs();
  }

  /// @brief Force the generation of an ack.
  void
  generate_ack()
  {
    // Add all currently known source identifiers to the ack to be sent.
    for (const auto& id_src : m_decoder.sources())
    {
      m_ack.source_ids().insert(m_ack.source_ids().end(), id_src.first);
    }

    // Ask packetizer to handle the bytes of the new ack (will be routed to user's handler).
    m_packetizer.write_ack(m_ack);
    ++m_nb_sent_ack;

    // Start a fresh new ack.
    m_ack.reset();
  }

  /// @brief Generate an ack if needed.
  void
  maybe_ack()
  {
    if (m_ack.nb_packets() >= m_ack_nb_packets)
    {
      generate_ack();
      m_last_ack_date = std::chrono::steady_clock::now();
    }
    else if (m_ack_frequency != std::chrono::milliseconds{0})
    {
      const auto now = std::chrono::steady_clock::now();
      if ((now - m_last_ack_date) >= m_ack_frequency)
      {
        generate_ack();
        m_last_ack_date = now;
      }
    }
  }

  /// @brief Set the frequency at which ack will be sent from the decoder to the encoder.
  ///
  /// If 0, ack won't be sent automatically. The method @ref decoder::generate_ack can still be
  /// called to force the generation of an ack.
  decoder&
  set_ack_frequency(std::chrono::milliseconds f)
  noexcept
  {
    m_ack_frequency = f;
    return *this;
  }

  /// @brief Get the frequency at which ack will be sent from the decoder to the encoder.
  std::chrono::milliseconds
  ack_frequency()
  const noexcept
  {
    return m_ack_frequency;
  }

  /// @brief Set how many packets to receive before an ack is sent from the decoder to the encoder.
  /// @pre @p nb > 0
  decoder&
  set_ack_nb_packets(std::uint16_t nb)
  noexcept
  {
    assert(nb > 0);
    m_ack_nb_packets = std::min(nb, static_cast<std::uint16_t>(128));
    return *this;
  }

  /// @brief Get how many packets to receive before an ack is sent from the decoder to the encoder.
  std::uint16_t
  ack_nb_packets()
  const noexcept
  {
    return m_ack_nb_packets;
  }

private:

  /// @brief Callback given to the real encoder to be notified when a source is processed.
  void
  handle_source(const detail::decoder_source& src)
  {
    // Ask user to read the bytes of this new source.
    m_data_handler(src.symbol(), src.symbol_size());

    // Send an ack if necessary.
    maybe_ack();
  }

private:

  /// @brief The Galois field size.
  const std::uint8_t m_galois_field_size;

  /// @brief The frequency at which ack will be sent back from the decoder to the encoder.
  std::chrono::milliseconds m_ack_frequency;

  /// @brief How many packets to receive before an ack is sent from the decoder to the encoder.
  std::uint16_t m_ack_nb_packets;

  /// @brief The last time an ack was sent.
  std::chrono::steady_clock::time_point m_last_ack_date;

  /// @brief Re-use the same memory to prepare an ack packet.
  detail::ack m_ack;

  /// @brief The component that rebuilds sources using repairs.
  detail::decoder m_decoder;

  /// @brief The user's handler to output data on network.
  packet_handler_type m_packet_handler;

  /// @brief The user's handler to read decoded data.
  data_handler_type m_data_handler;

  /// @brief How to read and write packets.
  detail::packetizer<packet_handler_type> m_packetizer;

  /// @brief The counter of received repairs.
  std::size_t m_nb_received_repairs;

  /// @brief The counter of received sources.
  std::size_t m_nb_received_sources;

  /// @brief The number of ack sent back to the encoder.
  std::size_t m_nb_sent_ack;

#ifdef NTC_DUMP_PACKETS
  std::ofstream m_dump_file;
#endif
};

/*------------------------------------------------------------------------------------------------*/

} // namespace ntc
