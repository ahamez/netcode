#include <chrono>
#include <iostream>
#include <memory>

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wconversion"
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
#define ASIO_STANDALONE
#include <asio.hpp>
#pragma GCC diagnostic pop

#include "netcode/encoder.hh"

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
    auto buffer = std::make_shared<std::vector<char>>(std::move(m_buffer));
    m_socket.async_send_to( asio::buffer(*buffer), m_endpoint
                          , [buffer](const asio::error_code& err, std::size_t len)
                            {
                              if (err)
                              {
                                throw std::runtime_error{err.message()};
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

class app_handler
{
public:

  app_handler(ntc::encoder<packet_handler>& encoder, udp::socket& socket, udp::endpoint& end)
    : m_encoder(encoder)
    , m_socket(socket)
    , m_endpoint(end)
    , m_data(buffer_size)
  {
    start();
  }

private:

  void
  start()
  {
    m_socket.async_receive_from( asio::buffer(m_data), m_endpoint
                               , [this](const asio::error_code& err, std::size_t len)
                                 {
                                   if (err)
                                   {
                                     throw std::runtime_error(err.message());
                                   }

                                   if (len > 0)
                                   {
                                     m_data.resize(len);
                                     m_encoder(std::move(m_data));
                                     m_data.resize(buffer_size);
                                   }

                                   start();
                                 }
                               );
  }

private:

  ntc::encoder<packet_handler>& m_encoder;
  udp::socket& m_socket;
  udp::endpoint& m_endpoint;
  ntc::data m_data;
};

/*------------------------------------------------------------------------------------------------*/

class netcode_handler
{
public:

  netcode_handler(asio::io_service& io, ntc::encoder<packet_handler>& encoder, udp::socket& socket, udp::endpoint& end)
    : m_encoder(encoder)
    , m_socket(socket)
    , m_endpoint(end)
    , m_packet(buffer_size)
   , m_stats_timer(io, std::chrono::seconds(5))
  {
    start();
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
                                     m_encoder(std::move(m_packet));
                                     m_packet.resize(buffer_size);
                                   }

                                   start();
                                 }
                               );
  }

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
                                 << "in  acks   : " << m_encoder.nb_received_acks() << '\n'
                                 << "out repairs: " << m_encoder.nb_sent_repairs() << '\n'
                                 << "out sources: " << m_encoder.nb_sent_sources() << '\n'
                                 << "window : " << m_encoder.window() << '\n'
                                 << "rate : " << m_encoder.rate() << '\n'
                                 << '\n'
                                 << std::endl;

                               start_stats_timer();
                             });
  }

private:

  ntc::encoder<packet_handler>& m_encoder;
  udp::socket& m_socket;
  udp::endpoint& m_endpoint;
  ntc::packet m_packet;
  asio::steady_timer m_stats_timer;
};

/*------------------------------------------------------------------------------------------------*/

int
main(int argc, char** argv)
{
  if (argc != 4)
  {
    std::cerr << "Usage\n";
    std::cerr << argv[0] << " app_port receiver_ip receiver_port\n";
    std::exit(1);
  }
  try
  {
    asio::io_service io;

    const auto app_port = static_cast<unsigned short>(std::atoi(argv[1]));
    const auto server_url = argv[2];
    const auto server_port = argv[3];

    udp::socket app_socket{io, udp::endpoint{udp::v4(), app_port}};
    app_socket.set_option(asio::socket_base::receive_buffer_size{1 << 21});
    udp::endpoint app_endpoint;

    udp::socket netcode_socket{io, udp::endpoint(udp::v4(), 0)};
    netcode_socket.set_option(asio::socket_base::send_buffer_size{1 << 21});
    udp::resolver resolver(io);
    udp::endpoint netcode_endpoint = *resolver.resolve({udp::v4(), server_url, server_port});

    ntc::encoder<packet_handler> encoder(8, packet_handler{netcode_socket, netcode_endpoint});
    encoder.set_window_size(256);
    encoder.set_adaptive(true);

    app_handler app{encoder, app_socket, app_endpoint};
    netcode_handler netcode{io, encoder, netcode_socket, netcode_endpoint};

    io.run();
  }
  catch (const std::exception& e)
  {
    std::cerr << e.what() << '\n';
  }
  return 0;
}

/*------------------------------------------------------------------------------------------------*/
