#include <cstdlib>
#include <iostream>

#include "accelerator/transcoder.hh"

/*------------------------------------------------------------------------------------------------*/

class server
{
public:

  server( const ntc::configuration& conf, std::uint16_t port, const std::string& app_addr
        , const std::string& app_port)
    : io_()
    , app_socket_(io_, udp::endpoint(udp::v4(), 0)) // a client socket
    , app_endpoint_()
    , socket_(io_, udp::endpoint{udp::v4(), port})  // a server socket
    , endpoint_()
    , transcoder_(conf, io_, app_socket_, app_endpoint_, socket_, endpoint_)
  {
    udp::resolver resolver(io_);
    app_endpoint_ = *resolver.resolve({udp::v4(), app_addr, app_port});
  }

  void
  operator()()
  {
    io_.run();
  }

private:

  /// @brief
  asio::io_service io_;

  /// @brief
  udp::socket app_socket_;

  /// @brief
  udp::endpoint app_endpoint_;

  /// @brief
  udp::socket socket_;

  /// @brief
  udp::endpoint endpoint_;

  /// @brief
  transcoder transcoder_;
};

/*------------------------------------------------------------------------------------------------*/

int
main(int argc, char** argv)
{
  if (argc != 4)
  {
    std::cerr << "Usage: " << argv[0] << " server_port app_ip app_port\n";
    return 1;
  }
  try
  {
    server s(ntc::configuration{}, std::atoi(argv[1]), argv[2], argv[3]);
    s();
  }
  catch (const std::exception& e)
  {
    std::cerr << e.what() << '\n';
  }
  return 0;
}

/*------------------------------------------------------------------------------------------------*/
