#include <cstdlib>
#include <iostream>

#define ASIO_STANDALONE
#define BOOST_DATE_TIME_NO_LIB
#define ASIO_HAS_BOOST_DATE_TIME
#include <asio.hpp>

#include <netcode/configuration.hh>

#include "accelerator/transcoder.hh"

/*------------------------------------------------------------------------------------------------*/

int
main(int argc, char** argv)
{
  const auto usage = [&]
  {
    std::cerr << "Usage\n";
    std::cerr << argv[0] << " server server_port app_ip app_port\n";
    std::cerr << argv[0] << " client app_port server_ip server_port\n";
    std::exit(1);
  };

  if (argc != 5)
  {
    usage();
  }
  try
  {
    asio::io_service io;

    // Connection with the application.
    udp::socket app_socket{io};
    udp::endpoint app_endpoint;

    // Encoded connection.
    udp::socket socket{io};
    udp::endpoint endpoint;

    // netcode configuration
    ntc::configuration conf;

    if (std::strncmp(argv[1], "server", 7) == 0)
    {
      const auto server_port = static_cast<unsigned short>(std::atoi(argv[2]));
      const auto app_url = argv[3];
      const auto app_port = argv[4];

      app_socket = udp::socket{io, udp::endpoint(udp::v4(), 0)}; // a client socket
      socket = udp::socket{io, udp::endpoint{udp::v4(), server_port}};  // a server socket

      udp::resolver resolver(io);
      app_endpoint = *resolver.resolve({udp::v4(), app_url, app_port});
    }
    else if (std::strncmp(argv[1], "client", 7) == 0)
    {
      const auto app_port = static_cast<unsigned short>(std::atoi(argv[2]));
      const auto server_url = argv[3];
      const auto server_port = argv[4];

      app_socket = udp::socket{io, udp::endpoint{udp::v4(), app_port}}; // a server socket
      socket = udp::socket{io, udp::endpoint(udp::v4(), 0)};            // a client socket

      udp::resolver resolver(io);
      endpoint = *resolver.resolve({udp::v4(), server_url, server_port});
    }
    else
    {
      usage();
    }

    transcoder t{conf, io, app_socket, app_endpoint, socket, endpoint};
    io.run();
  }
  catch (const std::exception& e)
  {
    std::cerr << e.what() << '\n';
  }
  return 0;
}

/*------------------------------------------------------------------------------------------------*/
