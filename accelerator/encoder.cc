#include <algorithm>
#include <iostream>
#include <memory>

#define ASIO_STANDALONE
#include <asio.hpp>
#include <netcode/encoder.hh>

/*------------------------------------------------------------------------------------------------*/

using asio::ip::udp;

/*------------------------------------------------------------------------------------------------*/

class transcoder
  : public std::enable_shared_from_this<transcoder>
{
public:

  /// @brief Constructor.
  transcoder(ntc::configuration conf, asio::io_service& io, std::uint16_t port)
    : io_{io}
    , as_server_socket_{io_, udp::endpoint{udp::v4(), port}}
    , as_server_endpoint_{}
    , as_client_socket_{io_}
    , as_client_endpoint_{}
    , encoder_{[this](const char* data, std::size_t len){encoder_callback(data, len);}, conf}
    , as_server_buffer_{}
    , as_server_nb_written_{0}
  {
//    udp::resolver resolver{io};
//    as_client_endpoint_ = *resolver.resolve({udp::v4(), "127.0.0.1", "12345"});
//    as_client_socket_.open(udp::v4());
    do_receive();
  }

private:

  void
  do_receive()
  {
    // sources
    as_server_socket_.async_receive_from( asio::buffer(as_server_buffer_, max_len)
                                        , as_server_endpoint_
                                        , [this, self = shared_from_this()]
                                          (const asio::error_code& err, std::size_t sz)
                                          {
                                            if (err)
                                            {
                                              std::cerr << err.value() << '\n';
                                              std::cerr << err.message() << '\n';
                                              throw "arg";
                                            }
                                            ntc::copy_symbol symbol{as_server_buffer_, sz};
                                            encoder_.commit(std::move(symbol));
                                          });

    // ack
//    as_client_socket_.async_receive_from( asio::buffer(as_client_buffer_, max_len)
//                                        , as_client_endpoint_
//                                        , [this](const asio::error_code& err, std::size_t sz)
//                                          {
//                                            if (err)
//                                            {
//                                              throw "arg2";
//                                            }
//                                            ntc::copy_packet packet{as_client_buffer_, sz};
//                                            encoder_.notify(std::move(packet));
//                                          });
  }

  /// @brief Callback for encoder.
  void
  encoder_callback(const char* data, std::size_t len)
  {
    if (data)
    {
      std::copy_n(data, len, as_server_buffer_ + as_server_nb_written_);
      as_server_nb_written_ += len;
    }
    else
    {
      // End of data, we can now send it.
      as_client_socket_.async_send_to( asio::buffer(as_server_buffer_, as_server_nb_written_)
                                     , as_client_endpoint_
                                     , [this](const asio::error_code&, std::size_t sz)
                                       {
                                         do_receive();
                                       });
      as_server_nb_written_ = 0;
    }
  }

private:

  /// @brief
  static constexpr auto max_len = 2048;

  /// @brief
  asio::io_service& io_;

  /// @brief
  udp::socket as_server_socket_;

  /// @brief
  udp::endpoint as_server_endpoint_;

  /// @brief
  udp::socket as_client_socket_;

  /// @brief
  udp::endpoint as_client_endpoint_;

  /// @brief
  ntc::encoder encoder_;

  /// @brief
  char as_server_buffer_[max_len];

  /// @brief
  std::size_t as_server_nb_written_;

  /// @brief
  char as_client_buffer_[max_len];

};

/*------------------------------------------------------------------------------------------------*/

int
main(int argc, char** argv)
{
  try
  {
    ntc::configuration conf;
    asio::io_service io;
    transcoder{conf, io, 8888};
    io.run();
  }
  catch (const std::exception& e)
  {
    std::cerr << e.what() << '\n';
  }
  return 0;
}

/*------------------------------------------------------------------------------------------------*/
