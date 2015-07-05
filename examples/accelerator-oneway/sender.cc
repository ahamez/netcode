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
    m_buffer.reserve(2048);
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
    m_socket.send_to(asio::buffer(m_buffer), m_endpoint);
    m_buffer.clear();
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
    : m_encoder{encoder}
    , m_socket{socket}
    , m_endpoint{end}
    , m_data(2048)
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
                                     m_encoder(std::move(m_data));
                                   }

                                   m_data.resize(2048);
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

  netcode_handler(ntc::encoder<packet_handler>& encoder, udp::socket& socket, udp::endpoint& end)
    : m_encoder{encoder}
    , m_socket{socket}
    , m_endpoint{end}
    , m_packet(2048)
  {
    start();
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
                                   }

                                   m_packet.resize(2048);
                                   start();
                                 }
                               );
  }

private:

  ntc::encoder<packet_handler>& m_encoder;
  udp::socket& m_socket;
  udp::endpoint& m_endpoint;
  ntc::packet m_packet;
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

    // Connection with the proxied application.
    udp::socket app_socket{io};
    udp::endpoint app_endpoint;

    // Encoded tunnel.
    udp::socket netcode_socket{io};
    udp::endpoint netcode_endpoint;

    const auto app_port = static_cast<unsigned short>(std::atoi(argv[1]));
    const auto server_url = argv[2];
    const auto server_port = argv[3];

    app_socket = udp::socket{io, udp::endpoint{udp::v4(), app_port}};
    netcode_socket = udp::socket{io, udp::endpoint(udp::v4(), 0)};
    udp::resolver resolver(io);
    netcode_endpoint = *resolver.resolve({udp::v4(), server_url, server_port});

    ntc::encoder<packet_handler> encoder(8, packet_handler{netcode_socket, netcode_endpoint});

    app_handler app{encoder, app_socket, app_endpoint};
    netcode_handler netcode{encoder, netcode_socket, netcode_endpoint};

    // Launch the event loop (runs forever).
    io.run();
  }
  catch (const std::exception& e)
  {
    std::cerr << e.what() << '\n';
  }
  return 0;
}

/*------------------------------------------------------------------------------------------------*/
