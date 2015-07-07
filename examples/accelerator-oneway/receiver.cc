#include <chrono>
#include <iostream>
#include <memory>

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wconversion"
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
#define ASIO_STANDALONE
#include <asio.hpp>
#pragma GCC diagnostic pop

#include "netcode/decoder.hh"

/*------------------------------------------------------------------------------------------------*/

static constexpr auto buffer_size = 4096;

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

/// @brief Called by encoder when a packet is ready to be written to the network
class packet_handler
{
public:

  packet_handler(const packet_handler&) = delete;
  packet_handler& operator=(const packet_handler&) = delete;

  packet_handler(packet_handler&&) = default;
  packet_handler& operator=(packet_handler&&) = default;

  packet_handler(udp::socket& sock, udp::endpoint& end)
    : m_socket(sock), m_endpoint(end), m_buffer()
  {
    m_buffer.reserve(buffer_size);
  }

  /// @brief This function is invoked repeatedly until the packet is complete
  void
  operator()(const char* data, std::size_t sz)
  {
    std::copy_n(data, sz, std::back_inserter(m_buffer));
  }

  /// @brief This function is invoked when the packet received by the previous callback is complete
  void
  operator()()
  {
    // End of packet, we can now send it.

    auto buffer = std::make_shared<std::vector<char>>(std::move(m_buffer));
    m_socket.async_send_to( asio::buffer(*buffer), m_endpoint
                          , [buffer](const asio::error_code& err, std::size_t len)
                            {
                              if (err)
                              {
                                throw std::runtime_error(err.message());
                              }

                              if (len != buffer->size())
                              {
                                throw std::runtime_error{"Invalid number of sent bytes"};
                              }
                            });
    m_buffer.reserve(buffer_size);
  }

private:

  udp::socket& m_socket;
  udp::endpoint& m_endpoint;

  std::vector<char> m_buffer;
};

/*------------------------------------------------------------------------------------------------*/

/// @brief Called by decoder when a data has been decoded or received
class data_handler
{
public:

  data_handler(udp::socket& sock, udp::endpoint& end)
    : m_socket(sock), m_endpoint(end)
  {}

  void
  operator()(const char* data, std::size_t sz)
  {
    auto buffer = std::make_shared<std::vector<char>>(data, data + sz);
    m_socket.async_send_to( asio::buffer(*buffer), m_endpoint
                          , [buffer](const asio::error_code& err, std::size_t len)
                            {
                              if (err)
                              {
                                throw std::runtime_error(err.message());
                              }

                              if (len != buffer->size())
                              {
                                throw std::runtime_error{"Invalid number of sent bytes"};
                              }
                            });
  }

private:

  udp::socket& m_socket;
  udp::endpoint& m_endpoint;
};

/*------------------------------------------------------------------------------------------------*/

class netcode_handler
{
public:

  netcode_handler( asio::io_service& io, ntc::decoder<packet_handler, data_handler>& decoder
                 , udp::socket& socket, udp::endpoint& end)
    : m_decoder(decoder)
    , m_socket(socket)
    , m_endpoint(end)
    , m_packet(buffer_size)
    , m_ack_timer(io, std::chrono::milliseconds{1})
    , m_tunnel_up(false)
    , m_stats_timer(io, std::chrono::seconds(5))
  {
    start();
    start_timer();
    start_stats_timer();
  }

private:

  void
  start()
  {
    m_socket.async_receive_from( asio::buffer(m_packet), m_endpoint
                               , [this](const asio::error_code& err, std::size_t len)
                                 {
                                   if (err)
                                   {
                                     throw std::runtime_error(err.message());
                                   }

                                   if (len > 0)
                                   {
                                     m_decoder(std::move(m_packet));
                                     m_tunnel_up = true;
                                     m_packet.resize(buffer_size);
                                   }

                                   start();
                                 });
  }

  void
  start_timer()
  {
    m_ack_timer.expires_from_now(std::chrono::milliseconds{1});
    m_ack_timer.async_wait([this](const asio::error_code& err)
                           {
                             if (err)
                             {
                               throw std::runtime_error(err.message());
                             }
                             if (m_tunnel_up)
                             {
                               m_decoder.generate_ack();
                             }
                             start_timer();
                           });
  }

  /// @brief Display statistics every 5 seconds
  void
  start_stats_timer()
  {
    m_stats_timer.expires_from_now(std::chrono::seconds(5));
    m_stats_timer.async_wait([this](const asio::error_code& err)
                             {
                               if (err)
                               {
                                 throw std::runtime_error(err.message());
                               }
                               std::cout
                                 << "out acks   : " << m_decoder.nb_sent_acks() << '\n'
                                 << "in  repairs: " << m_decoder.nb_received_repairs() << '\n'
                                 << "in  sources: " << m_decoder.nb_received_sources() << '\n'
                                 << "decoded: " << m_decoder.nb_decoded() << '\n'
                                 << "failed : " << m_decoder.nb_failed_full_decodings() << '\n'
                                 << "useless: " << m_decoder.nb_useless_repairs() << '\n'
                                 << "missing: " << m_decoder.nb_missing_sources() << '\n'
                                 << '\n'
                                 << std::endl;

                               start_stats_timer();
                             });
  }


private:

  ntc::decoder<packet_handler, data_handler>& m_decoder;
  udp::socket& m_socket;
  udp::endpoint& m_endpoint;
  ntc::packet m_packet;
  asio::steady_timer m_ack_timer;
  bool m_tunnel_up;
  asio::steady_timer m_stats_timer;
};

/*------------------------------------------------------------------------------------------------*/

int
main(int argc, char** argv)
{
  if (argc != 4)
  {
    std::cerr << "Usage\n";
    std::cerr << argv[0] << " receiver_port app_ip app_port\n";
    std::exit(1);
  }
  try
  {
    asio::io_service io;

    const auto netcode_port = static_cast<unsigned short>(std::atoi(argv[1]));
    const auto app_url = argv[2];
    const auto app_port = argv[3];

    udp::socket netcode_socket{io, udp::endpoint{udp::v4(), netcode_port}};
    udp::endpoint netcode_endpoint;

    udp::socket app_socket{io, udp::endpoint(udp::v4(), 0)};
    udp::resolver resolver(io);
    udp::endpoint app_endpoint = *resolver.resolve({udp::v4(), app_url, app_port});

    ntc::decoder<packet_handler, data_handler>
      decoder( 8, ntc::in_order::yes
             , packet_handler{netcode_socket, netcode_endpoint}
             , data_handler{app_socket, app_endpoint});


    decoder.set_ack_frequency(std::chrono::milliseconds{0});
    netcode_handler netcode{io, decoder, netcode_socket, netcode_endpoint};
    io.run();
  }
  catch (const ntc::packet_type_error& e)
  {
    std::cerr << "Invalid packet type " << +e.error_packet.data()[0] << '\n';
    std::exit(2);
  }
  catch (const std::exception& e)
  {
    std::cerr << e.what() << '\n';
    std::exit(3);
  }
  return 0;
}

/*------------------------------------------------------------------------------------------------*/
