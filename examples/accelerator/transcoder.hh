#pragma once

#include <algorithm> // copy_n
#include <chrono>
#include <memory>
#include <iostream>
#include <vector>

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wconversion"
#define ASIO_STANDALONE
#include <asio.hpp>
#pragma GCC diagnostic pop

#include <netcode/decoder.hh>
#include <netcode/dispatch.hh>
#include <netcode/encoder.hh>

/*------------------------------------------------------------------------------------------------*/

namespace asio {

/// @brief Make ntc::packet usable as a mutable buffer for Asio.
inline
mutable_buffers_1
buffer(ntc::packet& p)
{
  return mutable_buffers_1{mutable_buffer{p.size() ? p.data() : nullptr, p.size()}};
}

} // namespace asio

/*------------------------------------------------------------------------------------------------*/

using asio::ip::address_v4;
using asio::ip::udp;

/*------------------------------------------------------------------------------------------------*/

/// @brief The maximal size of an UDP packet
static constexpr auto buffer_size = 65504;

/*------------------------------------------------------------------------------------------------*/

/// @brief Called by encoder when a packet is ready to be written to the network
class packet_handler
{
private:

  udp::socket& socket;
  udp::endpoint& endpoint;

  std::vector<char> buffer;

public:

  packet_handler(const packet_handler&) = delete;
  packet_handler& operator=(const packet_handler&) = delete;

  packet_handler(packet_handler&&) = default;
  packet_handler& operator=(packet_handler&&) = default;

  packet_handler(udp::socket& sock, udp::endpoint& end)
    : socket(sock), endpoint(end), buffer()
  {}

  /// @brief This function is invoked repeatedly until the packet is complete
  void
  operator()(const char* data, std::size_t sz)
  {
    std::copy_n(data, sz, std::back_inserter(buffer));
  }

  /// @brief This function is invoked when the packet received by the previous callback is complete
  void
  operator()()
  {
    // End of packet, we can now send it.
    socket.send_to(asio::buffer(buffer), endpoint);
    buffer.clear();
  }
};

/*------------------------------------------------------------------------------------------------*/

/// @brief Called by decoder when a data has been decoded or received
struct data_handler
{
  udp::socket& socket;
  udp::endpoint& endpoint;

  data_handler(udp::socket& sock, udp::endpoint& end)
    : socket(sock), endpoint(end)
  {}

  void
  operator()(const char* data, std::size_t sz)
  noexcept
  {
    socket.send_to(asio::buffer(data, sz), endpoint);
  }
};

/*------------------------------------------------------------------------------------------------*/

/// @brief A side of the encoded tunnel.
///
/// It contains a decoder and an encoder to provide two-ways communications.
class transcoder
{
public:

  /// @brief Constructor
  transcoder( asio::io_service& io
            , udp::socket& app_socket
            , udp::endpoint& app_endpoint
            , udp::socket& socket
            , udp::endpoint& endpoint)
    : m_ack_timer(io, std::chrono::milliseconds(100))
    , m_stats_timer(io, std::chrono::seconds(5))
    , m_app_socket(app_socket)
    , m_app_endpoint(app_endpoint)
    , m_socket(socket)
    , m_endpoint(endpoint)
    , m_decoder(8, ntc::in_order::yes, packet_handler( m_socket, m_endpoint)
                                                     , data_handler(m_app_socket, m_app_endpoint))
    , m_encoder(8, packet_handler(m_socket, m_endpoint))
    , m_packet(buffer_size)
    , m_data(buffer_size)
    , m_other_side_seen(false)
  {
    // Deactivate automatic sending of ack by the library, we'll take care of it.
    m_decoder.set_ack_frequency(std::chrono::milliseconds{0});
    m_encoder.set_window_size(32);
    m_encoder.set_adaptive(true);

    // Start all handlers.
    start_handler();
    start_app_handler();
    start_timer_handler();
    start_stats_timer_handler();
  }

private:

  /// @brief Listen for incomging packets from the other side (decoder or encoder)
  void
  start_handler()
  {
    m_socket.async_receive_from( asio::buffer(m_packet)
                               , m_endpoint
                               , [this](const asio::error_code& err, std::size_t len)
                                 {
                                   if (err)
                                   {
                                     throw std::runtime_error(err.message());
                                   }

                                   m_packet.resize(len);

                                   m_other_side_seen = true;

                                   // The received packet might be for the encoder or the decoder.
                                   // The netcode library provides the following function to
                                   // dispatch to the appropriate component using the packet's type
                                   // (source, ack or repair).
                                   ntc::dispatch(m_encoder, m_decoder, std::move(m_packet));

                                   m_packet.resize(buffer_size);

                                   // Listen again for incoming packets.
                                   start_handler();
                                 });
  }

  /// @brief Listen for incoming data from the application
  void
  start_app_handler()
  {
    m_app_socket.async_receive_from( asio::buffer(m_data)
                                   , m_app_endpoint
                                   , [this](const asio::error_code& err, std::size_t len)
                                     {
                                       if (err)
                                       {
                                         throw std::runtime_error(err.message());
                                       }

                                       // m_data has been filled by asio, but we still need to tell
                                       // the netcode library how many bytes were really written.
                                       m_data.resize(len);

                                       // We can now safely give the data to the encoder.
                                       m_encoder(std::move(m_data));

                                       // To avoid allocating a new data, we reset m_data.
                                       m_data.resize(buffer_size);

                                       // Listen again for incoming data.
                                       start_app_handler();
                                     });
  }

  /// @brief Generate an ack every 100 ms
  void
  start_timer_handler()
  {
    m_ack_timer.expires_from_now(std::chrono::milliseconds(100));
    m_ack_timer.async_wait([this](const asio::error_code& err)
                      {
                        if (err)
                        {
                          throw std::runtime_error(err.message());
                        }
                        if (m_other_side_seen)
                        {
                          m_decoder.generate_ack();
                        }
                        start_timer_handler();
                      });
  }

  /// @brief Display statistics every 5 seconds
  void
  start_stats_timer_handler()
  {
    m_stats_timer.expires_from_now(std::chrono::seconds(5));
    m_stats_timer.async_wait([this](const asio::error_code& err)
                             {
                               if (err)
                               {
                                 throw std::runtime_error(err.message());
                               }
                               std::cout
                                 << "-- Encoder --\n"
                                 << "in  acks   : " << m_encoder.nb_received_acks() << '\n'
                                 << "out repairs: " << m_encoder.nb_sent_repairs() << '\n'
                                 << "out sources: " << m_encoder.nb_sent_sources() << '\n'
                                 << "window : " << m_encoder.window() << '\n'
                                 << "rate : " << m_encoder.rate() << '\n'
                                 << '\n'
                                 << "-- Decoder --\n"
                                 << "out acks   : " << m_decoder.nb_sent_acks() << '\n'
                                 << "in  repairs: " << m_decoder.nb_received_repairs() << '\n'
                                 << "in  sources: " << m_decoder.nb_received_sources() << '\n'
                                 << "decoded: " << m_decoder.nb_decoded() << '\n'
                                 << "failed : " << m_decoder.nb_failed_full_decodings() << '\n'
                                 << "useless: " << m_decoder.nb_useless_repairs() << '\n'
                                 << "missing: " << m_decoder.nb_missing_sources() << '\n'
                                 << '\n'
                                 << std::endl;

                               start_stats_timer_handler();
                             });
  }


private:

  /// @brief Timer to send ack
  asio::steady_timer m_ack_timer;

  /// @brief Timer to display statistics
  asio::steady_timer m_stats_timer;

  /// @brief The proxied application's socket
  udp::socket& m_app_socket;

  /// @brief The proxied application communication point
  udp::endpoint& m_app_endpoint;

  /// @brief The encoded tunnel socket
  udp::socket& m_socket;

  /// @brief The encoded tunnel communication point
  udp::endpoint& m_endpoint;

  /// @brief The decoder on this side of the tunnel
  ntc::decoder<packet_handler, data_handler> m_decoder;

  /// @brief The encoder on this side of the tunnel
  ntc::encoder<packet_handler> m_encoder;

  /// @brief Store packets received from the tunnel
  ntc::packet m_packet;

  /// @brief Store data received from the proxied application
  ntc::data m_data;

  /// @brief Set the first time a packet has been exchanged with the other side of the tunnel
  bool m_other_side_seen;
};

/*------------------------------------------------------------------------------------------------*/
