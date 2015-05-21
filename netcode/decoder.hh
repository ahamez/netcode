#pragma once

#include <chrono>
#include <functional>

#include "netcode/detail/decoder.hh"
#include "netcode/detail/packet_type.hh"
#include "netcode/detail/packetizer.hh"
#include "netcode/detail/repair.hh"
#include "netcode/detail/source.hh"
#include "netcode/code.hh"
#include "netcode/configuration.hh"

namespace ntc {

/*------------------------------------------------------------------------------------------------*/

/// @brief The class to interact with on the receiver side.
template < typename PacketHandler, typename DataHandler
         , typename Packetizer = detail::packetizer<PacketHandler>>
class decoder final
{
private:

  /// @brief The type of the component that de/serializes packets.
  using packetizer_type = Packetizer;

public:

  /// @brief The type of the handler that processes data ready to be sent on the network.
  using packet_handler_type = PacketHandler;

  /// @brief The type of the handler that processes decoded or received data.
  using data_handler_type = DataHandler;

public:

  /// @brief Can't copy-construct an encoder.
  decoder(const decoder&) = delete;

  /// @brief Can't copy an encoder.
  decoder& operator=(const decoder&) = delete;

  /// @brief Constructor.
  template <typename PacketHandler_, typename DataHandler_>
  decoder( PacketHandler_&& packet_handler, DataHandler_&& data_handler
         , configuration conf)
    : m_conf{conf}
    , m_last_ack_date(std::chrono::steady_clock::now())
    , m_ack{}
    , m_decoder{ conf.galois_field_size()
              , std::bind(&decoder::handle_source, this, std::placeholders::_1)
              , conf.in_order()}
    , m_packet_handler(std::forward<PacketHandler_>(packet_handler))
    , m_data_handler(std::forward<DataHandler_>(data_handler))
    , m_packetizer{m_packet_handler}
    , m_nb_received_repairs{0}
    , m_nb_received_sources{0}
    , m_nb_sent_ack{0}
  {
    // Let's reserve some memory for the ack, it will most likely avoid memory re-allocations.
    m_ack.source_ids().reserve(128);
  }

  /// @brief Constructor with a default configuration.
  template <typename PacketHandler_, typename DataHandler_>
  decoder(PacketHandler_&& packet_handler, DataHandler_&& data_handler)
    : decoder{ std::forward<PacketHandler_>(packet_handler)
             , std::forward<DataHandler_>(data_handler)
             , configuration{}}
  {}

  /// @brief Notify the encoder of a new incoming packet, typically from the network.
  /// @param packet The incoming packet.
  /// @return The number of bytes that have been read (0 if the packet was not decoded).
  ///
  /// To fulfill memory alignement requirements, a copy will occur.
  std::size_t
  operator()(const char* packet)
  {
    switch (detail::get_packet_type(packet))
    {
      case detail::packet_type::repair:
      {
        m_nb_received_repairs += 1;
        m_ack.nb_packets() += 1;
        auto res = m_packetizer.read_repair(packet);
        m_decoder(std::move(res.first));
        return res.second;
      }

      case detail::packet_type::source:
      {
        m_nb_received_sources += 1;
        m_ack.nb_packets() += 1;
        auto res = m_packetizer.read_source(packet);
        m_decoder(std::move(res.first));
        return res.second;
      }

      default: return 0ul;
    }
  }

  /// @brief Notify the encoder of a new incoming packet, typically from the network.
  /// @param packet The incoming packet stored in a vector.
  /// @return The number of bytes that have been read (0 if the packet was not decoded).
  std::size_t
  operator()(const std::vector<char>& buffer)
  {
    return operator()(buffer.data());
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

  /// @brief Get the number of times the full decodong failed.
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

  /// @brief Get the number of repairs that were dropped because they wereuseless.
  std::size_t
  nb_useless_repairs()
  const noexcept
  {
    return m_decoder.nb_useless_repairs();
  }

  /// @brief Force the sending of an ack.
  void
  send_ack()
  {
    // Add all currently known source identifiers to the ack to be sent.
    for (const auto& id_src : m_decoder.sources())
    {
      m_ack.source_ids().insert(m_ack.source_ids().end(), id_src.first);
    }

    // Ask packetizer to handle the bytes of the new ack (will be routed to user's handler).
    m_packetizer.write_ack(m_ack);
    m_nb_sent_ack +=1;

    // Start a fresh new ack.
    m_ack.reset();
  }

  /// @brief Send an ack if needed.
  void
  maybe_ack()
  {
    if (m_ack.nb_packets() >= m_conf.ack_nb_packets())
    {
      send_ack();
      m_last_ack_date = std::chrono::steady_clock::now();
    }
    else if (m_conf.ack_frequency() != std::chrono::milliseconds{0})
    {
      const auto now = std::chrono::steady_clock::now();
      if ((now - m_last_ack_date) >= m_conf.ack_frequency())
      {
        send_ack();
        m_last_ack_date = now;
      }
    }
  }

  /// @brief Get the configuration (mutable).
  configuration&
  conf()
  noexcept
  {
    return m_conf;
  }

  /// @brief Get the configuration.
  const configuration&
  conf()
  const noexcept
  {
    return m_conf;
  }

private:

  /// @brief Callback given to the real encoder to be notified when a source is processed.
  void
  handle_source(const detail::source& src)
  {
    // Ask user to read the bytes of this new source.
    m_data_handler(src.buffer().data(), src.user_size());

    // Send an ack if necessary.
    maybe_ack();
  }

private:

  /// @brief The configuration.
  configuration m_conf;

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

  /// @brief How to serialize packets.
  packetizer_type m_packetizer;

  /// @brief The counter of received repairs.
  std::size_t m_nb_received_repairs;

  /// @brief The counter of received sources.
  std::size_t m_nb_received_sources;

  /// @brief The number of ack sent back to the encoder.
  std::size_t m_nb_sent_ack;
};

/*------------------------------------------------------------------------------------------------*/

} // namespace ntc
