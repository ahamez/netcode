#include <algorithm>
#include <iostream>
#include <vector>

#define ASIO_STANDALONE
#define BOOST_DATE_TIME_NO_LIB
#define ASIO_HAS_BOOST_DATE_TIME
#include <asio.hpp>
#include <netcode/decoder.hh>
#include <netcode/encoder.hh>
#include <netcode/detail/packet_type.hh>

/*------------------------------------------------------------------------------------------------*/

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
    std::cout << "send " << written << " bytes to client\n";
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
    std::cout << "send " << sz << " bytes to app\n";
    socket.send_to(asio::buffer(data, sz), endpoint);
  }
};

/*------------------------------------------------------------------------------------------------*/

class transcoder
{
public:

  /// @brief Constructor.
  transcoder( ntc::configuration conf, asio::io_service& io
            , std::uint16_t port
            , const std::string& app_addr, const std::string& app_port
            )
    : io_(io)
//    , timer_(io_, boost::posix_time::milliseconds(100))
//    , timer_(io_, boost::posix_time::seconds(2))
    , app_socket_(io_, udp::endpoint(udp::v4(), 0)) // a client socket
    , app_endpoint_()
    , socket_(io_, udp::endpoint{udp::v4(), port})  // a listening socket
    , endpoint_()
    , decoder_(packet_handler(socket_, endpoint_), data_handler(app_socket_, app_endpoint_), conf)
    , encoder_(packet_handler(socket_, endpoint_))
    , packet_(max_len)
    , data_(max_len)
  {
    std::cout << "Waiting for client.\n";
    std::vector<char> buffer(max_len);
    socket_.receive_from(asio::buffer(buffer), endpoint_);
    std::cout << "Client connected.\n";

    udp::resolver resolver(io_);
    app_endpoint_ = *resolver.resolve({udp::v4(), app_addr, app_port});

    const std::string msg = "hello app\n";
    app_socket_.send_to(asio::buffer(msg), app_endpoint_);
    std::cout << "Message sent to app\n";

    start_handler();
    start_app_handler();
    start_timer_handler();
  }

private:

  // Will receive sources and repairs from encoder.
  void
  start_handler()
  {
    socket_.async_receive_from( asio::buffer(packet_.buffer(), max_len)
                              , endpoint_
                              , [this](const asio::error_code& err, std::size_t sz)
                                {
                                  if (err)
                                  {
                                    throw std::runtime_error(err.message());
                                  }

                                  if (sz == 0)
                                  {
                                    std::cout << "empty sz" << std::endl;
                                    start_handler();
                                    return;
                                  }

                                  std::cout << "received " << sz << " bytes from client"
                                            << std::endl;

                                  bool res = false;
                                  switch (ntc::detail::get_packet_type(packet_.buffer()))
                                  {
                                      case ntc::detail::packet_type::ack:
                                      {
                                        res = encoder_(std::move(packet_));
                                        break;
                                      }

                                      case ntc::detail::packet_type::repair:
                                      case ntc::detail::packet_type::source:
                                      {
                                        res = decoder_(std::move(packet_));
                                        break;
                                      }

                                      default:;
                                  }

                                  if (not res)
                                  {
                                    throw std::runtime_error("Invalid packet format");
                                  }

                                  // Prepare packet for next incoming.
                                  packet_.reset(max_len);

                                  // Listen again for incoming packets.
                                  start_handler();
                                });
  }

  void
  start_app_handler()
  {
    app_socket_.async_receive_from( asio::buffer(data_.buffer(), max_len)
                                  , app_endpoint_
                                  , [this](const asio::error_code& err, std::size_t sz)
                                    {
                                      if (err)
                                      {
                                        throw std::runtime_error(err.message());
                                      }

                                      std::cout << "received " << sz << " bytes from app"
                                                << std::endl;

                                      // Give the ack to decoder.
                                      data_.used_bytes() = sz;
                                      encoder_(std::move(data_));

                                      // Prepare ack for next incoming.
                                      data_.reset(max_len);

                                      // Listen again.
                                      start_app_handler();
                                    });

  }

  // Will force the decoder to send ack packets.
  void
  start_timer_handler()
  {
//    timer_.expires_from_now(boost::posix_time::seconds(2));
//    timer_.async_wait([this](const asio::error_code& err)
//                      {
//                        if (err)
//                        {
//                          throw std::runtime_error(err.message());
//                        }
//                        decoder_.send_ack();
//                        start_timer_handler();
//                      });
  }

private:

  /// @brief
  asio::io_service& io_;

  /// @brief
//  asio::deadline_timer timer_;

  /// @brief
  udp::socket app_socket_;

  /// @brief
  udp::endpoint app_endpoint_;

  /// @brief
  udp::socket socket_;

  /// @brief
  udp::endpoint endpoint_;

  /// @brief
  ntc::decoder<packet_handler, data_handler> decoder_;

  /// @brief
  ntc::encoder<packet_handler> encoder_;

  /// @brief
  ntc::packet packet_;

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
//    // Deactivate ack, we will will take care of it on our own.
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
