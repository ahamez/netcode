#include <algorithm>
#include <iostream>

#define ASIO_STANDALONE
#define BOOST_DATE_TIME_NO_LIB
#define ASIO_HAS_BOOST_DATE_TIME
//#include <boost/asio.hpp>
#include <asio.hpp>
#include <netcode/decoder.hh>

/*------------------------------------------------------------------------------------------------*/

//namespace asio = boost::asio;
using asio::ip::address_v4;
using asio::ip::udp;

/*------------------------------------------------------------------------------------------------*/

/// @brief
static constexpr auto max_len = 2048;

/*------------------------------------------------------------------------------------------------*/

/// @brief Called by decoder when a packet is ready to be written to the network.
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
    std::cout << "send " << written << " bytes of ack\n";
    // End of packet, we can now send it.
    socket.send_to(asio::buffer(buffer, written), endpoint);
    written = 0;
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
    std::cout << "send " << sz << " bytes of data\n";
    socket.send_to(asio::buffer(data, sz), endpoint);
  }
};

/*------------------------------------------------------------------------------------------------*/

class transcoder
{
public:

  /// @brief Constructor.
  transcoder( ntc::configuration conf, asio::io_service& io
            , std::uint16_t decoder_port
            , const std::string& app_addr, const std::string& app_port)
    : io_(io)
//    , timer_(io_, boost::posix_time::milliseconds(100))
    , timer_(io_, boost::posix_time::seconds(2))
    , encoder_socket_(io_, udp::endpoint{udp::v4(), decoder_port})
    , enccoder_endpoint_()
    , to_app_socket_(io_, udp::endpoint(udp::v4(), 0))
    , to_app_endpoint_()
    , decoder_( packet_handler(encoder_socket_, enccoder_endpoint_)
              , data_handler(to_app_socket_, to_app_endpoint_)
              , conf)
    , packet_(max_len)
    , encoder_connected_(false)
  {
    udp::resolver resolver(io_);
    to_app_endpoint_ = *resolver.resolve({udp::v4(), app_addr, app_port});

    std::cout << "Will listen on " << decoder_port << " and send to " << app_addr << ':'
              << app_port << '\n';
    start_server_handler();
    start_timer_handler();
  }

private:

  // Will receive sources and repairs from encoder.
  void
  start_server_handler()
  {
    encoder_socket_.async_receive_from( asio::buffer(packet_.buffer(), max_len)
                                      , enccoder_endpoint_
                                      , [this](const asio::error_code& err, std::size_t sz)
                                        {
                                           if (err)
                                           {
                                             throw std::runtime_error(err.message());
                                           }

                                          std::cout << "received " << sz << " bytes\n";
                                          encoder_connected_ = true;

                                          // Give the packet to decoder.
                                          const auto res = decoder_(std::move(packet_));
                                          if (not res)
                                          {
                                            throw std::runtime_error("Invalid packet format");
                                          }

                                          // Prepare packet for next incoming.
                                          packet_.reset(max_len);

                                          // Listen again for incoming packets.
                                          start_server_handler();
                                        });
  }

  // Will force the decoder to send ack packets.
  void
  start_timer_handler()
  {
    timer_.expires_from_now(boost::posix_time::seconds(2));
    timer_.async_wait([this](const asio::error_code& err)
                      {
                        if (err)
                        {
                          throw std::runtime_error(err.message());
                        }

                        if (encoder_connected_)
                        {
                          decoder_.send_ack();
                        }
                        start_timer_handler();
                      });
  }

private:

  /// @brief
  asio::io_service& io_;

  /// @brief
  asio::deadline_timer timer_;

  /// @brief
  udp::socket encoder_socket_;

  /// @brief
  udp::endpoint enccoder_endpoint_;

  /// @brief
  udp::socket to_app_socket_;

  /// @brief
  udp::endpoint to_app_endpoint_;

  /// @brief
  ntc::decoder<packet_handler, data_handler> decoder_;

  /// @brief
  ntc::packet packet_;

  ///
  bool encoder_connected_;

};

/*------------------------------------------------------------------------------------------------*/

int
main()
{
  try
  {
    ntc::configuration conf;
    // Deactivate ack, we will will take care of it on our own.
    conf.ack_frequency = std::chrono::milliseconds{0};
    asio::io_service io;
    transcoder t{conf, io, 9999, "127.0.0.1", "11111"};
    io.run();
  }
  catch (const std::exception& e)
  {
    std::cerr << e.what() << '\n';
  }
  return 0;
}

/*------------------------------------------------------------------------------------------------*/
