#include <array>
#include <chrono>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <string>

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wconversion"
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
#define ASIO_STANDALONE
#include <asio.hpp>
#pragma GCC diagnostic pop

#include "tools/loss/burst.hh"
#include "tools/loss/uniform.hh"

//Some global variables

//Frequency of report display
unsigned int display_timer = 5;

/*------------------------------------------------------------------------------------------------*/

class file_loss 
{
public:
  file_loss(std::string file)
  : input_file_{file}
  {
    loss_file.open(input_file_);
    if (!loss_file.is_open())
      std::cout << "File doesn't exist. Don't drop any packet" << std::endl;
  }

  ~file_loss()
  {
    loss_file.close();
  }

  bool
  operator()()
  noexcept
  {
      while (getline(loss_file,line)) {               
        if (line[0] == '0') {          
          return false; //Don't drop packet
        } else if (line[0] == '1') {          
          return true;
        } else {          
          return true;
        }            
      }          
    //Don't drop packet if end of file or the file does not exist
    return false;
  }


private:  
  std::string line;
  std::string input_file_;
  std::ifstream loss_file;  
};


/*------------------------------------------------------------------------------------------------*/

using asio::ip::address_v4;
using asio::ip::udp;

/*------------------------------------------------------------------------------------------------*/

static std::size_t a_to_b_losses = 0;
static std::size_t b_to_a_losses = 0;

static std::size_t a_to_b_total = 0;
static std::size_t b_to_a_total = 0;

/*------------------------------------------------------------------------------------------------*/

template <typename Loss>
void
listen( udp::socket& in_socket, udp::endpoint& in_endpoint
      , udp::socket& out_socket, udp::endpoint& out_endpoint
      , std::array<char, 4096>& buffer, Loss&& loss, std::size_t& losses, std::size_t& total)
{
  in_socket.async_receive_from( asio::buffer(buffer), in_endpoint
                              , [&](const asio::error_code& err, std::size_t sz)
                                {
                                  if (err)
                                  {
                                    throw std::runtime_error(err.message());
                                  }

                                  if (sz != 0) { //Apply forward/drop packet only if receiving a packet (size > 0)

                                    total += 1;

                                    if (not loss())
                                    {
                                      out_socket.send_to(asio::buffer(buffer, sz), out_endpoint);
                                    }
                                    else
                                    {
                                      losses += 1;
                                    }

                                  }

                                  listen( in_socket, in_endpoint, out_socket, out_endpoint, buffer
                                         , std::forward<Loss>(loss), losses, total);
                                }
                              );
}

/*------------------------------------------------------------------------------------------------*/

void
display(asio::steady_timer& timer)
{
  timer.expires_from_now(std::chrono::seconds(display_timer));
  timer.async_wait([&](const asio::error_code& err)
                    {
                      if (err)
                      {
                        throw std::runtime_error(err.message());
                      }
                      std::cout << " losses "
                                << a_to_b_losses << " , "  << b_to_a_losses
                                << " || total "
                                << a_to_b_total << " , " << b_to_a_total
                                << " || % "
                                << ( static_cast<float>(a_to_b_losses)
                                   / static_cast<float>(a_to_b_total)) * 100
                                << " , "
                                << ( static_cast<float>(b_to_a_losses)
                                   / static_cast<float>(b_to_a_total)) * 100
                                << std::endl;
                      display(timer);
                    }
                  );
}

/*------------------------------------------------------------------------------------------------*/

template <typename Loss>
void
proxy(unsigned short a_port, const std::string& b_ip, const std::string& b_port, Loss&& loss, Loss&& loss_inverse)
{
  asio::io_service io;

  udp::socket a_socket{io, udp::endpoint{udp::v4(), a_port}};
  udp::endpoint a_endpoint;

  udp::socket b_socket{io, udp::endpoint(udp::v4(), 0)};
  udp::resolver resolver(io);
  udp::endpoint b_endpoint = *resolver.resolve({udp::v4(), b_ip, b_port});

  asio::steady_timer timer{io, std::chrono::seconds(display_timer)};

  std::array<char, 4096> a_to_b;
  std::array<char, 4096> b_to_a;


  listen( a_socket, a_endpoint, b_socket, b_endpoint, a_to_b, std::forward<Loss>(loss)
        , a_to_b_losses, a_to_b_total);

  listen( b_socket, b_endpoint, a_socket, a_endpoint, b_to_a, std::forward<Loss>(loss_inverse)
       , b_to_a_losses, b_to_a_total);

  display(timer);

  io.run();
}

/*------------------------------------------------------------------------------------------------*/

int
main(int argc, char** argv)
{
  const auto usage = [&]
  {
    std::cerr << "Usage:\n";
    std::cerr << argv[0] << " from_port to_ip to_port burst p_good p_bad <p_good inverse> <p_bad inverse>\n";
    std::cerr << argv[0] << " from_port to_ip to_port uniform p <p inverse>\n";
    std::cerr << argv[0] << " from_port to_ip to_port file <filename> <filename inverse>\n";
    std::exit(1);
  };

  if (argc != 7 and argc != 9)
  {
    std::cout << "here\n";
    usage();
  }
  try
  {
    const auto from_port = static_cast<unsigned short>(std::atoi(argv[1]));
    const auto to_ip = argv[2];
    const auto to_port = argv[3];
    if (std::strncmp(argv[4], "burst", 6) == 0)
    {
      const auto p_good = static_cast<unsigned int>(std::atoi(argv[5]));
      const auto p_bad = static_cast<unsigned int>(std::atoi(argv[6]));
      const auto p_good_inv = static_cast<unsigned int>(std::atoi(argv[7]));
      const auto p_bad_inv = static_cast<unsigned int>(std::atoi(argv[8]));      

      proxy(from_port, to_ip, to_port, loss::burst{p_good, p_bad}, loss::burst{p_good_inv, p_bad_inv});
    }
    else if (std::strncmp(argv[4], "uniform", 8) == 0)
    {
      const auto p = static_cast<unsigned int>(std::atoi(argv[5]));
      const auto p_inv = static_cast<unsigned int>(std::atoi(argv[6]));

      proxy(from_port, to_ip, to_port, loss::uniform{100 - p}, loss::uniform{100 - p_inv});
    }
    else if (std::strncmp(argv[4], "file", 5) == 0)
    {
      
      proxy(from_port, to_ip, to_port, file_loss{argv[5]}, file_loss{argv[6]});
    }
    else
    {
      usage();
    }
  }
  catch (const std::exception& e)
  {
    std::cerr << e.what() << '\n';
  }
  return 0;
}

/*------------------------------------------------------------------------------------------------*/
