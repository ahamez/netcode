#include <iostream>

#include <boost/asio.hpp>

#include "netcode/packet.hh"
#include "netcode/detail/packetizer.hh"

/*------------------------------------------------------------------------------------------------*/

using boost::asio::ip::address_v4;
using boost::asio::ip::udp;

namespace boost {
namespace asio {

  inline
  mutable_buffers_1
  buffer(ntc::packet& p)
  {
    return mutable_buffers_1{mutable_buffer{p.size() ? p.data() : nullptr, p.size()}};
  }

} // namespace asio
} // namespace boost

/*------------------------------------------------------------------------------------------------*/

struct packet_handler
{
  void
  operator()(const char*, std::size_t)
  {}

  void
  operator()()
  {}
};

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

    boost::asio::io_service io;

    udp::socket in_socket{io, udp::endpoint{udp::v4(), in_port}};
    in_socket.set_option(boost::asio::socket_base::receive_buffer_size{8192*64});
    udp::endpoint in_endpoint;

    udp::socket out_socket{io, udp::endpoint(udp::v4(), 0)};
    out_socket.set_option(boost::asio::socket_base::send_buffer_size{8192*64});
    udp::resolver resolver(io);
    udp::endpoint out_endpoint = *resolver.resolve({udp::v4(), out_ip, out_port});

    packet_handler ph; // dummy
    ntc::detail::packetizer<packet_handler> packetizer{ph};

    while (true)
    {
      ntc::packet buffer(4096);

      const auto sz = in_socket.receive_from(boost::asio::buffer(buffer), in_endpoint);
      if (sz > 0)
      {
        if (buffer[0] == 2) // source
        {
          const auto src = packetizer.read_source(std::move(buffer)).first;
          out_socket.send_to(boost::asio::buffer(src.symbol(), src.symbol_size()), out_endpoint);
        }
      }
    }
  }
  catch (const std::exception& e)
  {
    std::cerr << e.what() << '\n';
  }
  return 0;
}

/*------------------------------------------------------------------------------------------------*/
