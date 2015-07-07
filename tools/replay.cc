#include <iostream>
#include <fstream>
#include <stdexcept>

#define NTC_DUMP_PACKETS
#include "netcode/packet.hh"
#undef NTC_DUMP_PACKETS
#include "netcode/decoder.hh"
#include "netcode/detail/serialize_packet.hh"

/*------------------------------------------------------------------------------------------------*/

struct packet_handler
{
  void
  operator()(const char*, std::size_t)
  noexcept
  {
  }

  void
  operator()()
  noexcept
  {
  }
};

/*------------------------------------------------------------------------------------------------*/

struct data_handler
{
  void
  operator()(const char*, std::size_t)
  noexcept
  {
  }
};

/*------------------------------------------------------------------------------------------------*/

int
main(int argc, const char** argv)
{
  const auto usage = [&]
  {
    std::cerr << "Usage:\n" << argv[0] << " dump_file\n";
    std::exit(1);
  };

  if (argc != 2)
  {
    usage();
  }
  try
  {
    auto dump_file = std::ifstream(argv[1]);
    if (not dump_file.is_open())
    {
      std::cerr << "Can't open " << argv[1] << '\n';
      std::exit(2);
    }

    packet_handler dummy;
    ntc::detail::packetizer<packet_handler> packetizer{dummy};

    ntc::decoder<packet_handler, data_handler> decoder{ 8, ntc::in_order::yes, packet_handler{}
                                                      , data_handler{}};

    auto counter = 0ul;
    while (true)
    {
      ntc::packet packet;
      ntc::detail::serialize_packet::read(dump_file);

      if (dump_file.peek() == std::char_traits<char>::eof())
      {
        break;
      }

      std::cout << counter++ << " ";
      switch (ntc::detail::get_packet_type(packet))
      {
        case ntc::detail::packet_type::ack:
        {
          throw std::runtime_error{"ack"};
          break;
        }

        case ntc::detail::packet_type::repair:
        {
          const auto r = packetizer.read_repair(ntc::packet{packet}).first;
          std::cout << "repair id " << r.id();
          break;
        }

        case ntc::detail::packet_type::source:
        {
          const auto s = packetizer.read_source(ntc::packet{packet}).first;
          std::cout << "source id " << s.id();
          break;
        }

        default:
          break;
      }
      std::cout << '\n';

      decoder(std::move(packet));
    }
  }
  catch (const ntc::packet_type_error& e)
  {
    std::cerr << "Invalid packet type " << +e.error_packet.data()[0] << '\n';
  }
  catch (const std::exception& e)
  {
    std::cerr << e.what() << '\n';
  }
  return 0;
}

/*------------------------------------------------------------------------------------------------*/
