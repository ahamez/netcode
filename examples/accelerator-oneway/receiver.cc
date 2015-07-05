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
    m_socket.send_to(asio::buffer(data, sz), m_endpoint);
  }

private:

  udp::socket& m_socket;
  udp::endpoint& m_endpoint;
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

    // Connection with the proxied application.
    udp::socket app_socket{io};
    udp::endpoint app_endpoint;

    // Encoded tunnel.
    udp::socket netcode_socket{io};
    udp::endpoint netcode_endpoint;

    const auto receiver_port = static_cast<unsigned short>(std::atoi(argv[1]));
    const auto app_url = argv[2];
    const auto app_port = argv[3];

    netcode_socket = udp::socket{io, udp::endpoint{udp::v4(), receiver_port}};
    app_socket = udp::socket{io, udp::endpoint(udp::v4(), 0)};
    udp::resolver resolver(io);
    app_endpoint = *resolver.resolve({udp::v4(), app_url, app_port});

    ntc::decoder<packet_handler, data_handler>
      decoder( 8, ntc::in_order::yes
             , packet_handler{netcode_socket, netcode_endpoint}
             , data_handler{app_socket, app_endpoint});

    while (true)
    {
      ntc::packet packet(2048);
      const auto len = netcode_socket.receive_from(asio::buffer(packet), netcode_endpoint);
      if (len == 0)
      {
        break;
      }
      decoder(std::move(packet));
     }
  }
  catch (const std::exception& e)
  {
    std::cerr << e.what() << '\n';
  }
  return 0;
}

/*------------------------------------------------------------------------------------------------*/
