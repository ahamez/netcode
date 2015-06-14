#include <algorithm>
#include <iostream>
#include <random>
#include <vector>

#include <netcode/decoder.hh>
#include <netcode/encoder.hh>

/*------------------------------------------------------------------------------------------------*/

static constexpr auto buffer_sz = 4096ul;

/*------------------------------------------------------------------------------------------------*/

class burst_loss
{
public:

  burst_loss(unsigned int good, unsigned int bad)
    : state_(state::good)
    , gen_()
    , dist_(1, 100)
    , good_(good)
    , bad_(bad)
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

  enum class state {good, bad};

  state state_;

  std::default_random_engine gen_;

  std::uniform_int_distribution<unsigned int> dist_;

  unsigned int good_;

  unsigned int bad_;
};

/*------------------------------------------------------------------------------------------------*/

struct packet_handler
{
  std::vector<std::vector<char>> buffer;

  packet_handler()
    : buffer()
  {}

  void
  operator()(const char* src, std::size_t len)
  {
    std::copy_n(src, len, std::back_inserter(buffer.back()));
  }

  void
  operator()()
  {
    buffer.emplace_back();
    buffer.back().reserve(buffer_sz);
  }
};

/*------------------------------------------------------------------------------------------------*/

struct in_order_data_handler
{
  std::size_t nb_received;
  std::uint32_t expected_id;
  const std::uint16_t packet_size;

  in_order_data_handler(std::uint16_t packet_sz)
    : nb_received(0), expected_id(0), packet_size(packet_sz)
  {}

  void
  operator()(const char* data, std::size_t len)
  {
    if (len != packet_size)
    {
      throw std::runtime_error("Invalid length");
    }
    const auto data_as_int = reinterpret_cast<const std::uint32_t*>(data);
    const auto data_id = *data_as_int;
    if (data_id != expected_id)
    {
      throw std::runtime_error( "Invalid id, expected " + std::to_string(expected_id)
                              + ", got " + std::to_string(data_id));
    }
    const auto all_equals = std::all_of( data_as_int, data_as_int + len/sizeof(std::uint32_t)
                                       , [&](std::uint32_t x){return x == data_id;});

    if (not all_equals)
    {
      throw std::runtime_error("Invalid data for packet " + std::to_string(data_id));
    }
    ++nb_received;
    expected_id += 1;
  }
};

/*------------------------------------------------------------------------------------------------*/

ntc::data
generate_data(std::uint32_t id, std::uint16_t packet_size)
{
  ntc::data data{packet_size};
  auto data_as_int = reinterpret_cast<std::uint32_t*>(data.buffer());
  std::fill(data_as_int, data_as_int + packet_size/sizeof(std::uint32_t), id);
  data.used_bytes() = packet_size;
  return data;
}

/*------------------------------------------------------------------------------------------------*/

int main()
{
  std::uint32_t id = 0;
  burst_loss loss{100, 0};
  std::size_t to_dec_nb_loss = 0;
  std::size_t to_enc_nb_loss = 0;
  std::size_t nb_packets = 999999;
  std::size_t ack_frequency = 50;
  std::uint16_t packet_size = 1024;
  if ((packet_size % sizeof(std::uint32_t)) != 0)
  {
    std::cerr << "Invalid packet size\n";
    return 1;
  }

  ntc::encoder<packet_handler> enc{8, packet_handler{}};
  ntc::decoder<packet_handler, in_order_data_handler> dec{ 8, true
                                                         , packet_handler{}
                                                         , in_order_data_handler{packet_size}};
  dec.set_ack_frequency(std::chrono::milliseconds{0});

  auto& enc_packet_handler = enc.packet_handler();
  auto& dec_packet_handler = dec.packet_handler();

  while (id < nb_packets)
  {
    if (id % 100 == 0)
    {
      std::cout << (id+1) << '/' << nb_packets
                << " | " << enc_packet_handler.buffer.size()
                << " | " << dec_packet_handler.buffer.size()
                << '\r';
      std::cout.flush();
    }

    enc_packet_handler.buffer.emplace_back();
    enc_packet_handler.buffer.back().reserve(buffer_sz);

    enc(generate_data(id++, packet_size));

    // Give packets to decoder, with a possible loss.
    for ( auto cit = enc_packet_handler.buffer.begin(), end = enc_packet_handler.buffer.end() - 1
        ; cit != end; ++cit)
    {
      const auto& packet = *cit;
      if (not loss())
      {
        dec(packet.data(), packet.size());
      }
      else
      {
        ++to_dec_nb_loss;
      }
    }
    enc_packet_handler.buffer.clear();

    // Possibly send an ack.
    if (((dec.nb_received_sources() + dec.nb_received_repairs()) % ack_frequency) == 0)
    {
      dec_packet_handler.buffer.emplace_back();
      dec_packet_handler.buffer.back().reserve(buffer_sz);

      dec.generate_ack();

      for ( auto cit = dec_packet_handler.buffer.begin(), end = dec_packet_handler.buffer.end() - 1
          ; cit != end; ++cit)
      {
        const auto& packet = *cit;
        if (not loss())
        {
          enc(packet.data(), packet.size());
        }
        else
        {
          ++to_enc_nb_loss;
        }
      }
      dec_packet_handler.buffer.clear();
    }
  }

  std::cout << "\n\nEncoder\n";
  std::cout << "Sent " << id << '\n';
  std::cout << "Lost " << to_dec_nb_loss << '\n';
  std::cout << '\n';

  std::cout << "Decoder\n";
  std::cout << "Handled data " << dec.data_handler().nb_received << '\n';
  std::cout << "Received repairs " << dec.nb_received_repairs() << '\n';
  std::cout << "Received sources " << dec.nb_received_sources() << '\n';
  std::cout << "Useless repairs " << dec.nb_useless_repairs() << '\n';
  std::cout << "Decoded " << dec.nb_decoded() << '\n';
  std::cout << '\n';

  return 0;
}

/*------------------------------------------------------------------------------------------------*/
