#include <algorithm>
#include <functional>
#include <iostream>
#include <mutex>
#include <queue>
#include <random>
#include <thread>
#include <vector>

#include <netcode/decoder.hh>
#include <netcode/encoder.hh>

/*------------------------------------------------------------------------------------------------*/

static constexpr auto buffer_sz = 4096ul;
using queue_type = std::queue<std::vector<char>>;

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
  std::vector<char> buffer;
  burst_loss& loss;
  bool lost_current_packet;
  std::size_t nb_loss;
  queue_type& queue;
  std::mutex& mutex;

  packet_handler(burst_loss& l, queue_type& q, std::mutex& m)
    : buffer(), loss(l), lost_current_packet(loss()), nb_loss(lost_current_packet ? 1u : 0u)
    , queue(q), mutex(m)
  {
    buffer.reserve(buffer_sz);
  }

  void
  operator()(const char* src, std::size_t len)
  {
    if (not lost_current_packet)
    {
      std::copy_n(src, len, std::back_inserter(buffer));
    }
  }

  void
  operator()()
  {
    {
      std::lock_guard<std::mutex> lock{mutex};
      if (not lost_current_packet)
      {
        queue.push(std::move(buffer));
      }
    }
    buffer.clear();
    buffer.reserve(buffer_sz);
    lost_current_packet = loss();
    nb_loss += lost_current_packet ? 1u : 0u;
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
  data.set_used_bytes(packet_size);
  return data;
}

/*------------------------------------------------------------------------------------------------*/

void
encoder( queue_type& to_dec, std::mutex& to_dec_mutex, queue_type& to_enc, std::mutex& to_enc_mutex
       , const bool& run, std::uint16_t packet_size)
{
  burst_loss loss{85, 15};
  ntc::encoder<packet_handler> enc{8, packet_handler{loss, to_dec, to_dec_mutex}};
  std::uint32_t id = 0;

  while (run)
  {
    enc(generate_data(id++, packet_size));

    // Read ack if any.
    {
      std::lock_guard<std::mutex> lock{to_enc_mutex};
      if (not to_enc.empty())
      {
        enc(to_enc.front().data(), to_enc.front().size());
        to_enc.pop();
      }
    }
  }

  std::lock_guard<std::mutex> out_lock{to_enc_mutex}; // to serialize output
  std::cout << "Encoder\n";
  std::cout << "Sent " << (id + 1) << '\n';
  std::cout << "Lost " << enc.packet_handler().nb_loss << '\n';
  std::cout << '\n';
}

/*------------------------------------------------------------------------------------------------*/

void
decoder( queue_type& to_dec, std::mutex& to_dec_mutex, queue_type& to_enc, std::mutex& to_enc_mutex
       , const bool& run, std::uint16_t packet_size)
{
  burst_loss loss{85, 15};
  ntc::decoder<packet_handler, in_order_data_handler>
    dec{8, true, packet_handler{loss, to_enc, to_enc_mutex}, in_order_data_handler{packet_size}};

  while (run)
  {
    // Read source or repair if any.
    {
      std::lock_guard<std::mutex> lock{to_dec_mutex};
      if (not to_dec.empty())
      {
        dec(to_dec.front().data(), to_dec.front().size());
        to_dec.pop();
      }
    }
  }

  std::lock_guard<std::mutex> out_lock{to_enc_mutex}; // to serialize output
  std::cout << "Decoder\n";
  std::cout << "Handled data " << dec.data_handler().nb_received << '\n';
  std::cout << "Received repairs " << dec.nb_received_repairs() << '\n';
  std::cout << "Received sources " << dec.nb_received_sources() << '\n';
  std::cout << "Useless repairs " << dec.nb_useless_repairs() << '\n';
  std::cout << "Decoded " << dec.nb_decoded() << '\n';
  std::cout << '\n';
}

/*------------------------------------------------------------------------------------------------*/

int main()
{
  std::uint16_t packet_size = 1024;
  if ((packet_size % sizeof(std::uint32_t)) != 0)
  {
    std::cerr << "Invalid packet size\n";
    return 1;
  }

  bool run = true;

  queue_type to_dec;
  std::mutex to_dec_mutex;

  queue_type to_enc;
  std::mutex to_enc_mutex;

  std::thread encoder_thread{ encoder, std::ref(to_dec), std::ref(to_dec_mutex), std::ref(to_enc)
                            , std::ref(to_enc_mutex), std::cref(run), packet_size};
  std::thread decoder_thread{ decoder, std::ref(to_dec), std::ref(to_dec_mutex), std::ref(to_enc)
                            , std::ref(to_enc_mutex), std::cref(run), packet_size};

  std::this_thread::sleep_for(std::chrono::seconds{10});

  run = false;

  encoder_thread.join();
  decoder_thread.join();

  return 0;
}

/*------------------------------------------------------------------------------------------------*/
