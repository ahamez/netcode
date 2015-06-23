#include <array>
#include <cassert>
#include <cstdio>
#include <cstdlib>
#include <iostream>
#include <random>
#include <stdexcept>
#include <string>

#include <fstream>

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wconversion"
#define ASIO_STANDALONE
#define BOOST_DATE_TIME_NO_LIB
#define ASIO_HAS_BOOST_DATE_TIME
#include <asio.hpp>
#pragma GCC diagnostic pop

//Some global variables

//Frequency of report display
unsigned int display_timer = 5;

/*------------------------------------------------------------------------------------------------*/

class burst_loss
{
public:

  burst_loss(unsigned int good, unsigned int bad)
    : state_{state::good}
    , gen_{}
    , dist_{1, 100}
    , good_{good}
    , bad_{bad}
  {}

  /// @return true if packet should be lost.
  bool
  operator()()
  noexcept
  {
    switch (state_)
    {
        case state::good:
        {
          if (dist_(gen_) < good_)
          {
            return false; // no loss
          }
          else
          {
            state_ = state::bad;
            return true; // loss
          }
        }

        case state::bad:
        {
          if (dist_(gen_) < bad_)
          {
            return true; // loss
          }
          else
          {
            state_ = state::good;
            return false; // no loss
          }
        }

        default: __builtin_unreachable();
    }
  }

private:

  /// @brief
  enum class state {good, bad};

  /// @brief
  state state_;

  /// @brief
  std::default_random_engine gen_;

  /// @brief
  std::uniform_int_distribution<unsigned int> dist_;

  /// @brief
  unsigned int good_;

  /// @bief
  unsigned int bad_;

};

/*------------------------------------------------------------------------------------------------*/

class uniform_loss
{
public:

  uniform_loss(unsigned int threshold)
    : gen_{}
    , dist_{1, 100}
    , threshold_{threshold}
  {
    assert(threshold_ > 0 and threshold < 100);
  }

  /// @return true if packet should be lost.
  bool
  operator()()
  noexcept
  {
    return dist_(gen_) > threshold_;
  }

private:


  /// @brief
  std::default_random_engine gen_;

  /// @brief
  std::uniform_int_distribution<unsigned int> dist_;

  /// @brief
  unsigned int threshold_;
};

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
display(asio::deadline_timer& timer)
{
  timer.expires_from_now(boost::posix_time::seconds(display_timer));
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
                                //<< '\r';
                                << std::endl;
                      //std::cout.flush();
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

  asio::deadline_timer timer{io, boost::posix_time::seconds(display_timer)};

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

      proxy(from_port, to_ip, to_port, burst_loss{p_good, p_bad}, burst_loss{p_good_inv, p_bad_inv});
      //proxy(from_port, to_ip, to_port, burst_loss{p_good, p_bad});
    }
    else if (std::strncmp(argv[4], "uniform", 8) == 0)
    {
      const auto p = static_cast<unsigned int>(std::atoi(argv[5]));
      const auto p_inv = static_cast<unsigned int>(std::atoi(argv[6]));

      proxy(from_port, to_ip, to_port, uniform_loss{100 - p}, uniform_loss{100 - p_inv});
      //proxy(from_port, to_ip, to_port, uniform_loss{100 - p});
    }
    else if (std::strncmp(argv[4], "file", 5) == 0)
    {
      
      proxy(from_port, to_ip, to_port, file_loss{argv[5]}, file_loss{argv[6]});
      //proxy(from_port, to_ip, to_port, file_loss{argv[5]});
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
