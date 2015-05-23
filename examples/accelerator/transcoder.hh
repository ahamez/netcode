#pragma once

#include <algorithm> // copy_n
#include <memory>
#include <iostream>
#include <vector>

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wconversion"
#define ASIO_STANDALONE
#define BOOST_DATE_TIME_NO_LIB
#define ASIO_HAS_BOOST_DATE_TIME
#include <asio.hpp>
#pragma GCC diagnostic pop

#include <netcode/decoder.hh>
#include <netcode/dispatch.hh>
#include <netcode/encoder.hh>

/*------------------------------------------------------------------------------------------------*/

using asio::ip::address_v4;
using asio::ip::udp;

/*------------------------------------------------------------------------------------------------*/

/// @brief
static constexpr auto buffer_size = 65504;

/*------------------------------------------------------------------------------------------------*/

/// @brief Called by encoder when a packet is ready to be written to the network.
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

  void
  operator()(const char* data, std::size_t sz)
  noexcept
  {
    std::copy_n(data, sz, std::back_inserter(buffer));
  }

  void
  operator()()
  noexcept
  {
    // End of packet, we can now send it.
    socket.send_to(asio::buffer(buffer), endpoint);
    buffer.clear();
  }
};

/*------------------------------------------------------------------------------------------------*/

/// @brief Called by decoder when a data has been decoded or received.
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

class transcoder
{
public:

  /// @brief Constructor.
  transcoder( asio::io_service& io
            , udp::socket& app_socket
            , udp::endpoint& app_endpoint
            , udp::socket& socket
            , udp::endpoint& endpoint)
    : m_timer(io, boost::posix_time::milliseconds(100))
    , m_stats_timer(io, boost::posix_time::seconds(5))
    , m_app_socket(app_socket)
    , m_app_endpoint(app_endpoint)
    , m_socket(socket)
    , m_endpoint(endpoint)
    , m_decoder(8, true, packet_handler(m_socket, m_endpoint), data_handler(m_app_socket, m_app_endpoint))
    , m_encoder(8, packet_handler(m_socket, m_endpoint))
    , m_packet()
    , m_data(buffer_size)
    , m_other_side_seen(false)
  {
    m_decoder.set_ack_frequency(std::chrono::milliseconds{0});
    m_encoder.set_window_size(32);
    m_encoder.set_adaptive(true);

    start_handler();
    start_app_handler();
    start_timer_handler();
    start_stats_timer_handler();
  }

private:

  void
  start_handler()
  {
    m_socket.async_receive_from( asio::buffer(m_packet, buffer_size)
                              , m_endpoint
                              , [this](const asio::error_code& err, std::size_t len)
                                {
                                  if (err)
                                  {
                                    throw std::runtime_error(err.message());
                                  }

                                  m_other_side_seen = true;
                                  ntc::dispatch(m_encoder, m_decoder, m_packet, len);

                                  // Listen again for incoming packets.
                                  start_handler();
                                });
  }

  void
  start_app_handler()
  {
    m_app_socket.async_receive_from( asio::buffer(m_data.buffer(), buffer_size)
                                  , m_app_endpoint
                                  , [this](const asio::error_code& err, std::size_t len)
                                    {
                                      if (err)
                                      {
                                        throw std::runtime_error(err.message());
                                      }
                                      m_data.used_bytes() = static_cast<std::uint16_t>(len);
                                      m_encoder(std::move(m_data));

                                      m_data.reset(buffer_size);

                                      // Listen again.
                                      start_app_handler();
                                    });
  }

  void
  start_timer_handler()
  {
    m_timer.expires_from_now(boost::posix_time::milliseconds(100));
    m_timer.async_wait([this](const asio::error_code& err)
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

  void
  start_stats_timer_handler()
  {
    m_stats_timer.expires_from_now(boost::posix_time::seconds(5));
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

  /// @brief
  asio::deadline_timer m_timer;

  /// @brief
  asio::deadline_timer m_stats_timer;

  /// @brief
  udp::socket& m_app_socket;

  /// @brief
  udp::endpoint& m_app_endpoint;

  /// @brief
  udp::socket& m_socket;

  /// @brief
  udp::endpoint& m_endpoint;

  /// @brief
  ntc::decoder<packet_handler, data_handler> m_decoder;

  /// @brief
  ntc::encoder<packet_handler> m_encoder;

  /// @brief
  char m_packet[buffer_size];

  /// @brief
  ntc::data m_data;

  /// @brief
  bool m_other_side_seen;
};

/*------------------------------------------------------------------------------------------------*/
