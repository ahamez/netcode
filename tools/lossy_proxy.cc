#include <array>
#include <cstdio>
#include <iostream>
#include <stdexcept>
#include <string>

#define ASIO_STANDALONE
#define BOOST_DATE_TIME_NO_LIB
#define ASIO_HAS_BOOST_DATE_TIME
#include <asio.hpp>

#include "tools/loss.hh"

/*------------------------------------------------------------------------------------------------*/

using asio::ip::address_v4;
using asio::ip::udp;

/*------------------------------------------------------------------------------------------------*/

static std::size_t a_to_b_losses = 0;
static std::size_t b_to_a_losses = 0;

/*------------------------------------------------------------------------------------------------*/

template <typename Loss>
void
listen( udp::socket& in_socket, udp::endpoint& in_endpoint
      , udp::socket& out_socket, udp::endpoint& out_endpoint
      , std::array<char, 4096>& buffer, Loss&& loss, std::size_t& losses)
{
  in_socket.async_receive_from( asio::buffer(buffer), in_endpoint
                              , [&](const asio::error_code& err, std::size_t sz)
                                {
                                  if (err)
                                  {
                                    throw std::runtime_error(err.message());
                                  }

                                  if (not loss())
                                  {
                                    out_socket.send_to(asio::buffer(buffer, sz), out_endpoint);
                                  }
                                  else
                                  {
                                    losses += 1;
                                  }

                                  listen( in_socket, in_endpoint, out_socket, out_endpoint, buffer
                                         , std::forward<Loss>(loss), losses);
                                }
                              );
}

/*------------------------------------------------------------------------------------------------*/

void
display(asio::deadline_timer& timer)
{
  timer.expires_from_now(boost::posix_time::seconds(1));
  timer.async_wait([&](const asio::error_code& err)
                    {
                      if (err)
                      {
                        throw std::runtime_error(err.message());
                      }
//                      std::printf("%lu | %lu\r", a_to_b_losses, b_to_a_losses);
                      std::cout << " losses " << a_to_b_losses << " | "  << b_to_a_losses << '\r';
                      std::cout.flush();
                      display(timer);
                    }
                  );
}

/*------------------------------------------------------------------------------------------------*/

template <typename Loss>
void
proxy(unsigned short a_port, const std::string& b_ip, const std::string& b_port, Loss&& loss)
{
  asio::io_service io;

  udp::socket a_socket{io, udp::endpoint{udp::v4(), a_port}};
  udp::endpoint a_endpoint;

  udp::socket b_socket{io, udp::endpoint(udp::v4(), 0)};
  udp::resolver resolver(io);
  udp::endpoint b_endpoint = *resolver.resolve({udp::v4(), b_ip, b_port});

  asio::deadline_timer timer{io, boost::posix_time::seconds(1)};

  std::array<char, 4096> a_to_b;
  std::array<char, 4096> b_to_a;

  listen(a_socket, a_endpoint, b_socket, b_endpoint, a_to_b, std::forward<Loss>(loss), a_to_b_losses);
  listen(b_socket, b_endpoint, a_socket, a_endpoint, b_to_a, std::forward<Loss>(loss), b_to_a_losses);
  display(timer);

  io.run();
}

/*------------------------------------------------------------------------------------------------*/

int
main(int argc, char** argv)
{
  if (argc != 4)
  {
    std::cerr << "Usage: " << argv[0] << "\n";
    return 1;
  }
  try
  {
    proxy(std::atoi(argv[1]), argv[2], argv[3], burst_loss{90, 50});
  }
  catch (const std::exception& e)
  {
    std::cerr << e.what() << '\n';
  }
  return 0;
}

/*------------------------------------------------------------------------------------------------*/
