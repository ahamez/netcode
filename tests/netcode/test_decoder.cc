#include <algorithm>
#include <thread>

#include "tests/catch.hpp"

#include "netcode/decoder.hh"
#include "netcode/encoder.hh"

/*------------------------------------------------------------------------------------------------*/

using namespace ntc;

/*------------------------------------------------------------------------------------------------*/

namespace /* unnamed */ {

struct handler
{
  char data[2048];
  std::size_t written = 0;

  void
  operator()(const char* src, std::size_t len)
  {
    std::copy_n(src, len, data + written);
    written += len;
  }

  void operator()() const noexcept {}
};


} // namespace unnamed

/*------------------------------------------------------------------------------------------------*/

TEST_CASE("Decoder gives a correct source to user")
{
  encoder<handler> enc{handler{}};
  decoder<handler, handler> dec{handler{}, handler{}};

  auto& enc_handler = enc.packet_handler();
  auto& dec_handler = dec.data_handler();

  const auto s0 = {'a', 'b', 'c'};

  enc(copy_data{begin(s0), end(s0)});

  // Send serialized data to decoder.
  dec(copy_packet{enc_handler.data, enc_handler.written});

  REQUIRE(dec_handler.written == s0.size());
  REQUIRE(std::equal(begin(s0), end(s0), dec_handler.data));
}

/*------------------------------------------------------------------------------------------------*/

TEST_CASE("Decoder repairs a lost source")
{
  configuration conf;
  conf.rate = 1; // A repair for a source.

  encoder<handler> enc{handler{}, conf};
  decoder<handler, handler> dec{handler{}, handler{}};

  auto& enc_handler = enc.packet_handler();
  auto& dec_data_handler = dec.data_handler();

  // Give a source to the encoder.
  const auto s0 = {'a', 'b', 'c'};
  enc(copy_data{begin(s0), end(s0)});

  const auto src_sz = sizeof(std::uint8_t)      // type
                    + sizeof(std::uint32_t)     // id
                    + sizeof(std::uint16_t)     // user data size
                    + s0.size();                // data

  // Skip first source.
  auto repair = copy_packet(enc_handler.data + src_sz, enc_handler.written - src_sz);

  // Send repair to decoder.
  REQUIRE(dec(std::move(repair)));
  REQUIRE(dec.nb_received_repairs() == 1);
  REQUIRE(dec.nb_received_sources() == 0);
  REQUIRE(dec.nb_decoded() == 1);
  REQUIRE(dec_data_handler.written == s0.size());
}

/*------------------------------------------------------------------------------------------------*/

TEST_CASE("Decoder generate correct ack")
{
  configuration conf;
  conf.rate = 100; // Make sure no repairs are sent.
  conf.ack_frequency = std::chrono::milliseconds{100};

  encoder<handler> enc{handler{}, conf};
  decoder<handler, handler> dec{handler{}, handler{}};

  auto& enc_handler = enc.packet_handler();
  auto& dec_packet_handler = dec.packet_handler();

  // Give a source to the encoder.
  const auto s0 = {'a', 'b', 'c'};
  enc(copy_data{begin(s0), end(s0)});
  REQUIRE(enc.window() == 1);

  // Send source to decoder.
  REQUIRE(dec(copy_packet(enc_handler.data, enc_handler.written)));

  SECTION("Force ack")
  {
    // Now force the sending of an ack.
    dec.send_ack();
    REQUIRE(dec.nb_sent_acks() == 1);

    // Sent it to the encoder.
    REQUIRE(enc(copy_packet(dec_packet_handler.data, dec_packet_handler.written)));
    REQUIRE(enc.window() == 0); // Source was correctly removed from the encoder window.
  }

  SECTION("Wait for trigger")
  {
    // Wait long enough just to be sure.
    std::this_thread::sleep_for(std::chrono::milliseconds{200});
    dec.maybe_ack();
    REQUIRE(dec.nb_sent_acks() == 1);

    // Sent it to the encoder.
    REQUIRE(enc(copy_packet(dec_packet_handler.data, dec_packet_handler.written)));
    REQUIRE(enc.window() == 0); // Source was correctly removed from the encoder window.
  }
}

/*------------------------------------------------------------------------------------------------*/
