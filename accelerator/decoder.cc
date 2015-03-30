//#include <algorithm>
//
//#define ASIO_STANDALONE
//#include <asio.hpp>
//#include <netcode/decoder.hh>
//
///*------------------------------------------------------------------------------------------------*/
//
//using asio::ip::udp;
//
///*------------------------------------------------------------------------------------------------*/
//
//class transcoder
//{
//public:
//
//  /// @brief Constructor.
//  transcoder(ntc::configuration conf, asio::io_service& io, std::uint16_t port)
//    : io_{io}
//    , server_socket_{io_, udp::endpoint{udp::v4(), port}}
//    , client_socket_{io_, udp::endpoint{udp::v4(), 0}}
//    , decoder_endpoint_{}
//    , encoder_{[this](const char* data, std::size_t len){callback(data, len);}, conf}
//    , buffer_{}
//    , nb_written_{0}
//  {
//    udp::resolver resolver{io};
//    decoder_endpoint_ = *resolver.resolve({udp::v4(), "127.0.0.1", "9999"});
//  }
//
//  /// @brief Main loop.
//  void
//  operator()()
//  {
//    while (true)
//    {
//      ntc::symbol symbol{max_len};
//      udp::endpoint sender_endpoint;
//      const auto len = server_socket_.receive_from( asio::buffer(symbol.buffer(), max_len)
//                                                  , sender_endpoint);
//      symbol.set_nb_written_bytes(len);
//      encoder_.commit(std::move(symbol));
//    }
//  }
//
//private:
//
//  /// @brief Callback for encoder.
//  void
//  callback(const char* data, std::size_t len)
//  {
//    if (data)
//    {
//      std::copy_n(data, len, buffer_ + nb_written_);
//      nb_written_ += len;
//    }
//    else
//    {
//      // End of data, we can now send it.
//      client_socket_.send_to(asio::buffer(buffer_, nb_written_), decoder_endpoint_);
//      nb_written_ = 0;
//    }
//  }
//
//private:
//
//  /// @brief
//  static constexpr auto max_len = 2048;
//
//  /// @brief
//  asio::io_service& io_;
//
//  /// @brief
//  udp::socket server_socket_;
//
//  /// @brief
//  udp::socket client_socket_;
//
//  /// @brief
//  udp::endpoint decoder_endpoint_;
//
//  /// @brief
//  ntc::encoder encoder_;
//
//  /// @brief
//  char buffer_[max_len];
//
//  /// @brief
//  std::size_t nb_written_;
//};
//
///*------------------------------------------------------------------------------------------------*/
//
//int
//main(int argc, char** argv)
//{
//  ntc::configuration conf;
//  asio::io_service io;
//  transcoder{conf, io, 8888}();
//  return 0;
//}
//
///*------------------------------------------------------------------------------------------------*/
