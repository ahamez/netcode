#include <algorithm>

#include "tests/catch.hpp"
#include "tests/netcode/common.hh"

#include "netcode/encoder.hh"

/*------------------------------------------------------------------------------------------------*/

using namespace ntc;

/*------------------------------------------------------------------------------------------------*/

namespace /* unnamed */ {

struct dummy_handler
{
  void operator()(const char*, std::size_t) const noexcept {}
  void operator()()                         const noexcept {} // end of data
};


} // namespace unnamed

/*------------------------------------------------------------------------------------------------*/

TEST_CASE("Encoder's window size")
{
  const auto data = std::vector<char>(100, 'x');

  configuration conf;
  conf.set_rate(1);
  encoder<dummy_handler> encoder{dummy_handler{}, conf};

  auto data0 = ntc::data(data.begin(), data.begin() + 13);
  encoder(std::move(data0));
  REQUIRE(encoder.window() == 1);

  auto data1 = ntc::data(data.begin(), data.begin() + 17);
  encoder(std::move(data1));
  REQUIRE(encoder.window() == 2);

  auto data2 = ntc::data(data.begin(), data.begin() + 17);
  encoder(std::move(data2));
  REQUIRE(encoder.window() == 3);
}

/*------------------------------------------------------------------------------------------------*/

TEST_CASE("Encoder can limit the window size")
{
  const auto data = std::vector<char>(100, 'x');

  configuration conf;
  conf.set_window_size(4);
  encoder<dummy_handler> encoder{dummy_handler{}, conf};

  auto data0 = ntc::data(data.begin(), data.begin() + 8);
  encoder(std::move(data0));

  data0 = ntc::data(data.begin(), data.begin() + 7);
  encoder(std::move(data0));

  data0 = ntc::data(data.begin(), data.begin() + 23);
  encoder(std::move(data0));

  data0 = ntc::data(data.begin(), data.begin() + 76);
  encoder(std::move(data0));

  data0 = ntc::data(data.begin(), data.begin() + 1);
  encoder(std::move(data0));
  REQUIRE(encoder.window() == 4);

  data0 = ntc::data(data.begin(), data.begin() + 32);
  encoder(std::move(data0));
  REQUIRE(encoder.window() == 4);
}

/*------------------------------------------------------------------------------------------------*/

TEST_CASE("Encoder generates repairs")
{
  const auto data = std::vector<char>(100, 'x');

  configuration conf;
  conf.set_rate(5);
  encoder<dummy_handler> encoder{dummy_handler{}, conf};

  for (auto i = 0ul; i < 100; ++i)
  {
    encoder(ntc::data(data.begin(), data.begin() + 1));
  }
  REQUIRE(encoder.nb_sent_repairs() == (100/5 /*code rate*/));
}

/*------------------------------------------------------------------------------------------------*/

TEST_CASE("Encoder correctly handles new incoming packets")
{
  configuration conf;
  conf.set_rate(5);
  encoder<dummy_handler> encoder{dummy_handler{}, conf};

  // First, add some sources.
  for (auto i = 0ul; i < 4; ++i)
  {
    auto data = ntc::data{512};
    data.used_bytes() = 512;
    encoder(std::move(data));
  }
  REQUIRE(encoder.window() == 4);

  struct handler
  {
    char pkt[2048];
    std::size_t written = 0ul;

    void
    operator()(const char* src, std::size_t len)
    {
      std::copy_n(src, len, pkt + written);
      written += len;
    }

    void operator()() const noexcept {} // end of data
  };

  // Directly use the serializer that would have been called by the decoder.
  handler h;
  detail::packetizer<handler> serializer{h};

  SECTION("incoming ack")
  {
    // Create an ack for some sources, with a wrong id, just to try.
    const auto ack = detail::ack{{0,2,9}, 0};

    // Serialize the ack.
    serializer.write_ack(ack);

    // Finally, notify the encoder.
    const auto result = encoder(h.pkt, 2048);
    REQUIRE(result);

    // The number of sources should have decreased.
    REQUIRE(encoder.window() == 2);
  }

  SECTION("incoming ack 2")
  {
    // Create an ack for some sources, with a wrong id, just to try.
    const auto ack0 = detail::ack{{0,2,9}, 0};

    // Serialize the ack.
    serializer.write_ack(ack0);

    // Finally, notify the encoder.
    const auto result0 = encoder(h.pkt, 2048);
    REQUIRE(result0);

    // The number of sources should have decreased.
    REQUIRE(encoder.window() == 2);

    /// Reset handler.
    h.written = 0;

    // Create an ack for some sources, with an already deleted source.
    const auto ack1 = detail::ack{{0}, 0};

    // Serialize the ack.
    serializer.write_ack(ack1);

    // Finally, notify the encoder.
    const auto result1 = encoder(h.pkt, 2048);
    REQUIRE(result1);

    // The number of sources should have decreased.
    REQUIRE(encoder.window() == 2);

    /// Reset handler.
    h.written = 0;
    
    // Create an ack for some sources, with a source that wasn't deleted before.
    const auto ack2 = detail::ack{{1}, 0};

    // Serialize the ack.
    serializer.write_ack(ack2);

    // Finally, notify the encoder.
    const auto result2 = encoder(h.pkt, 2048);
    REQUIRE(result2);

    // The number of sources should have decreased.
    REQUIRE(encoder.window() == 1);
  }

  SECTION("incoming repair")
  {
    // Create a repair.
    const auto repair = detail::repair{0};

    // Serialize the repair.
    serializer.write_repair(repair);

    // Finally, notify the encoder.
    const auto result = encoder(h.pkt, 2048);
    REQUIRE(not result);

    // The number of sources should not have decreased.
    REQUIRE(encoder.window() == 4);
  }

  SECTION("incoming source")
  {
    // Create a source.
    const auto source = detail::source{0, {}, 0};

    // Serialize the source.
    serializer.write_source(source);

    // Finally, notify the encoder.
    const auto result = encoder(h.pkt, 2048);
    REQUIRE(not result);

    // The number of sources should not have decreased.
    REQUIRE(encoder.window() == 4);
  }
}

/*------------------------------------------------------------------------------------------------*/

namespace /* unnamed */ {

struct my_handler
{
  char data[2048];
  std::size_t written = 0;
  std::size_t nb_packets = 0;

  void
  operator()(const char* src, std::size_t len)
  {
    std::copy_n(src, len, data + written);
    written += len;
  }

  void operator()()
  noexcept
  {
    // end of data
    nb_packets += 1;
  }
};

} // namespace unnamed

/*------------------------------------------------------------------------------------------------*/

TEST_CASE("Encoder sends correct sources")
{
  encoder<my_handler> enc{my_handler{}};

  auto& enc_handler = enc.packet_handler();

  const auto s0 = {'A', 'B', 'C'};

  enc(data{begin(s0), end(s0)});
  REQUIRE(enc_handler.written == ( sizeof(std::uint8_t)      // type
                                 + sizeof(std::uint32_t)     // id
                                 + sizeof(std::uint16_t)     // user data size
                                 + s0.size()                 // data
                                 ));
  REQUIRE(std::equal( begin(s0), end(s0)
                    , enc_handler.data + sizeof(std::uint8_t) + sizeof(std::uint32_t)
                                       + sizeof(std::uint16_t)
                    ));

}

/*------------------------------------------------------------------------------------------------*/

TEST_CASE("Encoder sends repairs")
{
  configuration conf;
  conf.set_rate(1); // A repair for a source

  encoder<my_handler> enc{my_handler{}, conf};

  auto& enc_handler = enc.packet_handler();

  const auto s0 = {'a', 'b', 'c'};

  enc(data{begin(s0), end(s0)});
  const auto src_sz = sizeof(std::uint8_t)      // type
                    + sizeof(std::uint32_t)     // id
                    + sizeof(std::uint16_t)     // user data size
                    + s0.size();                // data

  REQUIRE(enc_handler.nb_packets == 2 /* 1 source +  1 repair */);
  REQUIRE(detail::get_packet_type(enc_handler.data + src_sz) == detail::packet_type::repair);
}

/*------------------------------------------------------------------------------------------------*/

TEST_CASE("Decoder: invalid memory access scenerio")
{
  // A bug occurred when the pre-allocated size of a data was larger than the size of the cached
  // repair in the encoder.
  static constexpr auto max_len = 4096;

  configuration conf;
  conf.set_rate(5);
  conf.set_ack_frequency(std::chrono::milliseconds{0});

  encoder<packet_handler> enc{packet_handler{}, conf};

  auto& enc_handler = enc.packet_handler();

  // Packets will be stored in enc_handler.vec.
  const std::vector<char> x = {'x', '\n'};
  const std::vector<char> empty = {'\n'};

  {
    auto data_ptr = std::make_shared<ntc::data>(max_len);
    std::copy(x.begin(),  x.end(), data_ptr->buffer());
    data_ptr->used_bytes() = static_cast<std::uint16_t>(x.size());
    enc(std::move(*data_ptr));
  }
  {
    auto data_ptr = std::make_shared<ntc::data>(max_len);
    std::copy(empty.begin(),  empty.end(), data_ptr->buffer());
    data_ptr->used_bytes() = static_cast<std::uint16_t>(empty.size());
    enc(std::move(*data_ptr));
  }
  {
    auto data_ptr = std::make_shared<ntc::data>(max_len);
    std::copy(empty.begin(),  empty.end(), data_ptr->buffer());
    data_ptr->used_bytes() = static_cast<std::uint16_t>(empty.size());
    enc(std::move(*data_ptr));
  }
  {
    auto data_ptr = std::make_shared<ntc::data>(max_len);
    std::copy(empty.begin(),  empty.end(), data_ptr->buffer());
    data_ptr->used_bytes() = static_cast<std::uint16_t>(empty.size());
    enc(std::move(*data_ptr));
  }
  {
    auto data_ptr = std::make_shared<ntc::data>(max_len);
    std::copy(empty.begin(),  empty.end(), data_ptr->buffer());
    data_ptr->used_bytes() = static_cast<std::uint16_t>(empty.size());
    enc(std::move(*data_ptr));
  }
  REQUIRE(enc_handler.nb_packets() == 6);
  REQUIRE(enc.nb_sent_repairs() == 1);
  REQUIRE(enc.nb_sent_sources() == 5);
  REQUIRE(enc.nb_received_acks() == 0);
}

/*------------------------------------------------------------------------------------------------*/

TEST_CASE("Non systematic encoder")
{
  configuration conf;
  conf.set_code_type(code::non_systematic);
  conf.set_rate(3);

  encoder<packet_handler> encoder{packet_handler{}, conf};
  auto& enc_handler = encoder.packet_handler();

  const auto data = std::vector<char>(100, 'x');

  auto data0 = ntc::data(data.begin(), data.begin() + 7);
  encoder(std::move(data0));
  REQUIRE(encoder.nb_sent_sources() == 0);
  REQUIRE(encoder.window() == 1);
  REQUIRE(encoder.nb_sent_repairs() == 1);

  data0 = ntc::data(data.begin(), data.begin() + 23);
  encoder(std::move(data0));
  REQUIRE(encoder.nb_sent_sources() == 0);
  REQUIRE(encoder.window() == 2);
  REQUIRE(encoder.nb_sent_repairs() == 2);

  data0 = ntc::data(data.begin(), data.begin() + 76);
  encoder(std::move(data0));
  REQUIRE(encoder.nb_sent_sources() == 0);
  REQUIRE(encoder.window() == 3);
  REQUIRE(encoder.nb_sent_repairs() == 4);

  data0 = ntc::data(data.begin(), data.begin() + 1);
  encoder(std::move(data0));
  REQUIRE(encoder.nb_sent_sources() == 0);
  REQUIRE(encoder.window() == 4);
  REQUIRE(encoder.nb_sent_repairs() == 5);

  REQUIRE(enc_handler.nb_packets() == 5 /* 5 repairs */);
  REQUIRE(detail::get_packet_type(enc_handler.vec[0].data()) == detail::packet_type::repair);
  REQUIRE(detail::get_packet_type(enc_handler.vec[1].data()) == detail::packet_type::repair);
  REQUIRE(detail::get_packet_type(enc_handler.vec[2].data()) == detail::packet_type::repair);
  REQUIRE(detail::get_packet_type(enc_handler.vec[3].data()) == detail::packet_type::repair);
  REQUIRE(detail::get_packet_type(enc_handler.vec[4].data()) == detail::packet_type::repair);
}

/*------------------------------------------------------------------------------------------------*/
