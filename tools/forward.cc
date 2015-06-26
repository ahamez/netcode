#include <array>
#include <iostream>
#include <stdexcept>

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wconversion"
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
#define ASIO_STANDALONE
#include <asio.hpp>
#pragma GCC diagnostic pop

/*------------------------------------------------------------------------------------------------*/

using asio::ip::address_v4;
using asio::ip::udp;

/*------------------------------------------------------------------------------------------------*/

int
main(int argc, const char** argv)
{
  const auto usage = [&]
  {
    std::cerr << "Usage:\n" << argv[0] << " from_port to_ip to_port\n";
    std::exit(1);
  };

  if (argc != 4)
  {
    usage();
  }
  try
  {
    const auto in_port = static_cast<unsigned short>(std::atoi(argv[1]));
    const auto out_ip = argv[2];
    const auto out_port = argv[3];

    asio::io_service io;

    udp::socket in_socket{io, udp::endpoint{udp::v4(), in_port}};
    udp::endpoint in_endpoint;

    udp::socket out_socket{io, udp::endpoint(udp::v4(), 0)};
    udp::resolver resolver(io);
    udp::endpoint out_endpoint = *resolver.resolve({udp::v4(), out_ip, out_port});

    std::array<char, 4096> buffer;

    while (true)
    {
      udp::endpoint sender_endpoint;
      const auto sz = in_socket.receive_from(asio::buffer(buffer), in_endpoint);
      out_socket.send_to(asio::buffer(buffer, sz), out_endpoint);
    }
  }
  catch (const std::exception& e)
  {
    std::cerr << e.what() << '\n';
  }
  return 0;
}

/*------------------------------------------------------------------------------------------------*/
