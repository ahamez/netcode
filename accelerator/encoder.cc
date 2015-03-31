#include <algorithm>
#include <iostream>

#define ASIO_STANDALONE
#include <asio.hpp>
#include <netcode/encoder.hh>

/*------------------------------------------------------------------------------------------------*/

using asio::ip::address_v4;
using asio::ip::udp;

/*------------------------------------------------------------------------------------------------*/

/// @brief
static constexpr auto max_len = 2048;

/*------------------------------------------------------------------------------------------------*/

/// @brief Called by encoder when a packet is ready to be written to the network.
struct packet_handler
{
  udp::socket& socket;
  udp::endpoint& endpoint;

  char buffer[max_len];
  std::size_t written;

  packet_handler(udp::socket& sock, udp::endpoint& end)
    : socket(sock), endpoint(end), buffer(), written(0)
  {}

  void
  operator()(const char* data, std::size_t sz)
  noexcept
  {
    std::copy_n(data, sz, buffer + written);
    written += sz;
  }

  void
  operator()()
  noexcept
  {
    std::cout << "send " << written << " bytes of packet\n";
    // End of packet, we can now send it.
    socket.send_to(asio::buffer(buffer, written), endpoint);
    written = 0;
  }
};

/*------------------------------------------------------------------------------------------------*/

class transcoder
{
public:

  /// @brief Constructor.
  transcoder( ntc::configuration conf, asio::io_service& io
            , std::uint16_t server_port
            , const std::string& decoder_addr, const std::string& decoder_port)
    : io_(io)
    , from_app_socket_(io_, udp::endpoint{udp::v4(), server_port})
    , from_ap_endpoint_()
    , decoder_socket_(io_, udp::endpoint(udp::v4(), 0))
    , decoder_endpoint_()
    , encoder_(packet_handler(decoder_socket_, decoder_endpoint_), conf)
    , ack_(max_len)
    , data_(max_len)
  {
    udp::resolver resolver(io_);
    decoder_endpoint_ = *resolver.resolve({udp::v4(), decoder_addr, decoder_port});

    std::cout << "Will listen on " << server_port << " and send to " << decoder_addr << ':'
              << decoder_port << '\n';
    start_server_handler();
    start_client_handler();
  }

private:

  void
  start_server_handler()
  {
    from_app_socket_.async_receive_from( asio::buffer(data_.buffer(), max_len)
                                       , from_ap_endpoint_
                                       , [this](const asio::error_code& err, std::size_t sz)
                                         {
                                           if (err)
                                           {
                                             throw std::runtime_error(err.message());
                                           }

                                           std::cout << "received " << sz << " bytes of data\n";

                                           // Give the data to decoder.
                                           data_.used_bytes() = sz;
                                           encoder_(std::move(data_));

                                           // Prepare data for next incoming.
                                           data_.reset(max_len);

                                           // Listen again for incoming data.
                                           start_server_handler();
                                          });
  }

  // Will receive ack from decoder.
  void
  start_client_handler()
  {
    decoder_socket_.async_receive_from( asio::buffer(ack_.buffer(), max_len)
                                      , decoder_endpoint_
                                      , [this](const asio::error_code& err, std::size_t sz)
                                        {
                                          if (err)
                                          {
                                            throw std::runtime_error(err.message());
                                          }

                                          std::cout << "received " << sz << " bytes of ack\n";

                                          // Give the ack to decoder.
                                          encoder_(std::move(ack_));

                                          // Prepare ack for next incoming.
                                          ack_ = ntc::packet{max_len};

                                          // Listen again for incoming acks.
                                          start_client_handler();
                                        });
  }

private:

  /// @brief
  asio::io_service& io_;

  /// @brief
  udp::socket from_app_socket_;

  /// @brief
  udp::endpoint from_ap_endpoint_;

  /// @brief
  udp::socket decoder_socket_;

  /// @brief
  udp::endpoint decoder_endpoint_;

  /// @brief
  ntc::encoder<packet_handler> encoder_;

  /// @brief
  ntc::packet ack_;

  /// @brief
  ntc::data data_;
};

/*------------------------------------------------------------------------------------------------*/

int
main()
{
  try
  {
    ntc::configuration conf;
    asio::io_service io;
    transcoder t{conf, io, 8888, "127.0.0.1", "9999"};
    io.run();
  }
  catch (const std::exception& e)
  {
    std::cerr << e.what() << '\n';
  }
  return 0;
}

/*------------------------------------------------------------------------------------------------*/
