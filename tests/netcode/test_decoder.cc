#include <algorithm>
#include <thread>
#include <vector>

#include "tests/catch.hpp"
#include "tests/netcode/common.hh"

#include "netcode/decoder.hh"
#include "netcode/encoder.hh"
#include "netcode/detail/packet_type.hh"

/*------------------------------------------------------------------------------------------------*/

using namespace ntc;

/*------------------------------------------------------------------------------------------------*/

TEST_CASE("Decoder gives a correct source to user")
{
  encoder<packet_handler> enc{packet_handler{}};
  decoder<packet_handler, data_handler> dec{packet_handler{}, data_handler{}};

  auto& enc_packet_handler = enc.packet_handler();
  auto& dec_data_handler = dec.data_handler();

  const auto s0 = {'a', 'b', 'c'};

  enc(data{begin(s0), end(s0)});

  // Send serialized data to decoder.
  dec(packet{enc_packet_handler.vec[0].data(), enc_packet_handler.vec[0].size()});

  REQUIRE(dec_data_handler.vec[0].size() == s0.size());
  REQUIRE(std::equal(begin(s0), end(s0), begin(dec_data_handler.vec[0])));
}

/*------------------------------------------------------------------------------------------------*/

TEST_CASE("Decoder repairs a lost source")
{
  configuration conf;
  conf.rate = 1; // A repair for a source.

  encoder<packet_handler> enc{packet_handler{}, conf};
  decoder<packet_handler, data_handler> dec{packet_handler{}, data_handler{}};

  auto& enc_packet_handler = enc.packet_handler();
  auto& dec_data_handler = dec.data_handler();

  // Give a source to the encoder.
  const auto s0 = {'a', 'b', 'c'};
  enc(data{begin(s0), end(s0)});

  // Skip first source.
  auto repair = packet(enc_packet_handler.vec[1].data(), enc_packet_handler.vec[1].size());

  // Send repair to decoder.
  REQUIRE(dec(std::move(repair)));
  REQUIRE(dec.nb_received_repairs() == 1);
  REQUIRE(dec.nb_received_sources() == 0);
  REQUIRE(dec.nb_decoded() == 1);
  REQUIRE(dec_data_handler.vec[0].size() == s0.size());
  REQUIRE(std::equal(begin(s0), end(s0), begin(dec_data_handler.vec[0])));
}

/*------------------------------------------------------------------------------------------------*/

TEST_CASE("Decoder generate correct ack")
{
  configuration conf;
  conf.rate = 100; // Make sure no repairs are sent.
  conf.ack_frequency = std::chrono::milliseconds{100};

  encoder<packet_handler> enc{packet_handler{}, conf};
  decoder<packet_handler, data_handler> dec{packet_handler{}, data_handler{}};

  auto& enc_packet_handler = enc.packet_handler();
  auto& dec_packet_handler = dec.packet_handler();

  // Give a source to the encoder.
  const auto s0 = {'a', 'b', 'c'};
  enc(data{begin(s0), end(s0)});
  REQUIRE(enc.window() == 1);

  // Send source to decoder.
  dec(packet(enc_packet_handler.vec[0].data(), enc_packet_handler.vec[0].size()));

  SECTION("Force ack")
  {
    // Now force the sending of an ack.
    dec.send_ack();
    REQUIRE(dec.nb_sent_acks() == 1);

    // Sent it to the encoder.
    enc(packet(dec_packet_handler.vec[0].data(), dec_packet_handler.vec[0].size()));
    REQUIRE(enc.window() == 0); // Source was correctly removed from the encoder window.
  }

  SECTION("Wait for trigger")
  {
    // Wait long enough just to be sure.
    std::this_thread::sleep_for(std::chrono::milliseconds{200});
    dec.maybe_ack();
    REQUIRE(dec.nb_sent_acks() == 1);

    // Sent it to the encoder.
    enc(packet(dec_packet_handler.vec[0].data(), dec_packet_handler.vec[0].size()));
    REQUIRE(enc.window() == 0); // Source was correctly removed from the encoder window.
  }
}

/*------------------------------------------------------------------------------------------------*/

TEST_CASE("Decoder: lost packet with an encoder's limited window")
{
  configuration conf;
  conf.rate = 4;
  conf.window = 3;
  conf.ack_frequency = std::chrono::milliseconds{0};

  encoder<packet_handler> enc{packet_handler{}, conf};
  decoder<packet_handler, data_handler> dec{packet_handler{}, data_handler{}, conf};

  auto& enc_handler = enc.packet_handler();
  auto& dec_data_handler = dec.data_handler();

  // Packets will be stored in enc_handler.vec.
  const auto s0 = {'a', 'b', 'c'};
  enc(data{begin(s0), end(s0)});
  REQUIRE(enc.window() == 1);

  const auto s1 = {'d', 'e', 'f'};
  enc(data{begin(s1), end(s1)});
  REQUIRE(enc.window() == 2);

  const auto s2 = {'g', 'h', 'i'};
  enc(data{begin(s2), end(s2)});
  REQUIRE(enc.window() == 3);

  const auto s3 = {'j', 'k', 'l'};
  enc(data{begin(s3), end(s3)});
  REQUIRE(enc.window() == 3);

  REQUIRE(enc_handler.nb_packets() == 5 /* 4 src + 1 repair */);
  REQUIRE(detail::get_packet_type(enc_handler.vec[0].data()) == detail::packet_type::source);
  REQUIRE(detail::get_packet_type(enc_handler.vec[1].data()) == detail::packet_type::source);
  REQUIRE(detail::get_packet_type(enc_handler.vec[2].data()) == detail::packet_type::source);
  REQUIRE(detail::get_packet_type(enc_handler.vec[3].data()) == detail::packet_type::source);
  REQUIRE(detail::get_packet_type(enc_handler.vec[4].data()) == detail::packet_type::repair);

  // Now send to decoder.
  // Lost first source.
  dec(packet{enc_handler.vec[1].data(), enc_handler.vec[1].size()});
  dec(packet{enc_handler.vec[2].data(), enc_handler.vec[2].size()});
  dec(packet{enc_handler.vec[3].data(), enc_handler.vec[3].size()});
  // The arrival of the repair should decode the lost source.
  dec(packet{enc_handler.vec[4].data(), enc_handler.vec[4].size()});
  REQUIRE(dec.nb_received_sources() == 3);
  REQUIRE(dec.nb_received_repairs() == 1);
  REQUIRE(dec.nb_missing_sources() == 0);
  REQUIRE(dec.nb_decoded() == 0);

  // They were correctly given to the user handler.
  REQUIRE(dec_data_handler.vec.size() == 3);
  REQUIRE(std::equal(begin(s1), end(s1), begin(dec_data_handler.vec[0])));
  REQUIRE(std::equal(begin(s2), end(s2), begin(dec_data_handler.vec[1])));
  REQUIRE(std::equal(begin(s3), end(s3), begin(dec_data_handler.vec[2])));
}

/*------------------------------------------------------------------------------------------------*/
