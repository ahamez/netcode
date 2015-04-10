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
    : conf_{conf}
    , last_ack_date_(std::chrono::steady_clock::now())
    , ack_{}
    , decoder_{ conf.galois_field_size
              , std::bind(&decoder::handle_source, this, std::placeholders::_1)}
    , packet_handler_(std::forward<PacketHandler_>(packet_handler))
    , data_handler_(std::forward<DataHandler_>(data_handler))
    , packetizer_{packet_handler_}
    , nb_received_repairs_{0}
    , nb_received_sources_{0}
    , nb_handled_sources_{0}
    , nb_sent_ack_{0}
  {
    // Let's reserve some memory for the ack, it will most likely avoid memory re-allocations.
    ack_.source_ids().reserve(128);
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
        nb_received_repairs_ += 1;
        auto res = packetizer_.read_repair(packet);
        decoder_(std::move(res.first));
        return res.second;
      }

      case detail::packet_type::source:
      {
        nb_received_sources_ += 1;
        auto res = packetizer_.read_source(packet);
        decoder_(std::move(res.first));
        return res.second;
      }

      default: return 0ul;
    }
  }

  /// @brief Get the data handler.
  const packet_handler_type&
  packet_handler()
  const noexcept
  {
    return packet_handler_;
  }

  /// @brief Get the data handler.
  packet_handler_type&
  packet_handler()
  noexcept
  {
    return packet_handler_;
  }

  /// @brief Get the data handler.
  const data_handler_type&
  data_handler()
  const noexcept
  {
    return data_handler_;
  }

  /// @brief Get the data handler.
  data_handler_type&
  data_handler()
  noexcept
  {
    return data_handler_;
  }

  /// @brief Get the total number of decoded sources.
  std::size_t
  nb_decoded()
  const noexcept
  {
    return nb_handled_sources_ - nb_received_sources_;
  }

  /// @brief Get the number of times the full decodong failed.
  std::size_t
  nb_failed_full_decodings()
  const noexcept
  {
    return decoder_.nb_failed_full_decodings();
  }

  /// @brief Get the current number of missing sources.
  std::size_t
  nb_missing_sources()
  const noexcept
  {
    return decoder_.missing_sources().size();
  }

  /// @brief Get the total number of received repairs.
  std::size_t
  nb_received_repairs()
  const noexcept
  {
    return nb_received_repairs_;
  }

  /// @brief Get the total number of received sources.
  std::size_t
  nb_received_sources()
  const noexcept
  {
    return nb_received_sources_;
  }

  /// @brief Get the number of sent ack.
  std::size_t
  nb_sent_acks()
  const noexcept
  {
    return nb_sent_ack_;
  }

  /// @brief Get the number of repairs that were dropped because they wereuseless.
  std::size_t
  nb_useless_repairs()
  const noexcept
  {
    return decoder_.nb_useless_repairs();
  }

  /// @brief Force the sending of an ack.
  void
  send_ack()
  {
    // Add all currently known source identifiers to the ack to be sent.
    for (const auto& id_src : decoder_.sources())
    {
      ack_.source_ids().insert(ack_.source_ids().end(), id_src.first);
    }

    // Ask packetizer to handle the bytes of the new ack (will be routed to user's handler).
    packetizer_.write_ack(ack_);
    nb_sent_ack_ +=1;

    // Start a fresh new ack.
    ack_.reset();
  }

  /// @brief Send an ack if needed.
  void
  maybe_ack()
  {
    if (conf_.ack_frequency != std::chrono::milliseconds{0})
    {
      // Do we need to send an ack?
      const auto now = std::chrono::steady_clock::now();
      if ((now - last_ack_date_) >= conf_.ack_frequency)
      {
        send_ack();
        last_ack_date_ = now;
      }
    }
  }

  /// @brief Get the configuration (read-only).
  const configuration&
  conf()
  const noexcept
  {
    return conf_;
  }

  /// @brief Get the configuration.
  configuration&
  conf()
  noexcept
  {
    return conf_;
  }

private:

  /// @brief Callback given to the real encoder to be notified when a source is processed.
  void
  handle_source(const detail::source& src)
  {
    nb_handled_sources_ += 1;

    // Ask user to read the bytes of this new source.
    data_handler_(src.buffer().data(), src.user_size());

    // Send an ack if necessary.
    maybe_ack();
  }

private:

  /// @brief The configuration.
  configuration conf_;

  /// @brief The last time an ack was sent.
  std::chrono::steady_clock::time_point last_ack_date_;

  /// @brief Re-use the same memory to prepare an ack packet.
  detail::ack ack_;

  /// @brief The component that rebuilds sources using repairs.
  detail::decoder decoder_;

  /// @brief The user's handler to output data on network.
  packet_handler_type packet_handler_;

  /// @brief The user's handler to read decoded data.
  data_handler_type data_handler_;

  /// @brief How to serialize packets.
  packetizer_type packetizer_;

  /// @brief The counter of received repairs.
  std::size_t nb_received_repairs_;

  /// @brief The counter of received sources.
  std::size_t nb_received_sources_;

  /// @brief The counter of decoded sources.
  std::size_t nb_handled_sources_;

  /// @brief The number of ack sent back to the encoder.
  std::size_t nb_sent_ack_;
};

/*------------------------------------------------------------------------------------------------*/

} // namespace ntc
