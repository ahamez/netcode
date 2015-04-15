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
  dec(enc_packet_handler.vec[0].data());

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
  auto repair = enc_packet_handler.vec[1].data();

  // Send repair to decoder.
  REQUIRE(dec(repair));
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
  dec(enc_packet_handler.vec[0].data());

  SECTION("Force ack")
  {
    // Now force the sending of an ack.
    dec.send_ack();
    REQUIRE(dec.nb_sent_acks() == 1);

    // Sent it to the encoder.
    enc(dec_packet_handler.vec[0].data());
    REQUIRE(enc.window() == 0); // Source was correctly removed from the encoder window.
  }

  SECTION("Wait for trigger")
  {
    // Wait long enough just to be sure.
    std::this_thread::sleep_for(std::chrono::milliseconds{200});
    dec.maybe_ack();
    REQUIRE(dec.nb_sent_acks() == 1);

    // Sent it to the encoder.
    enc(dec_packet_handler.vec[0].data());
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
  dec(enc_handler.vec[1].data());
  dec(enc_handler.vec[2].data());
  dec(enc_handler.vec[3].data());
  // Because the encoder's window is 3, the incoming repair only encode s1, s2 and s3. Thus,
  // s0 is completely lost and cannot be recovered.
  dec(enc_handler.vec[4].data());
  REQUIRE(dec.nb_received_sources() == 3);
  REQUIRE(dec.nb_received_repairs() == 1);
  REQUIRE(dec.nb_missing_sources() == 0);
  REQUIRE(dec.nb_decoded() == 0);

  // Sources were correctly given to the user handler.
  REQUIRE(dec_data_handler.vec.size() == 3);
  REQUIRE(std::equal(begin(s1), end(s1), begin(dec_data_handler.vec[0])));
  REQUIRE(std::equal(begin(s2), end(s2), begin(dec_data_handler.vec[1])));
  REQUIRE(std::equal(begin(s3), end(s3), begin(dec_data_handler.vec[2])));
}

/*------------------------------------------------------------------------------------------------*/

TEST_CASE("Decoder: non systematic code")
{
  configuration conf;
  conf.rate = 4;
  conf.code_type = code::non_systematic;
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

  // Only repairs
  REQUIRE(enc_handler.nb_packets() == 5 /* repairs */);
  REQUIRE(detail::get_packet_type(enc_handler.vec[0].data()) == detail::packet_type::repair);
  REQUIRE(detail::get_packet_type(enc_handler.vec[1].data()) == detail::packet_type::repair);
  REQUIRE(detail::get_packet_type(enc_handler.vec[2].data()) == detail::packet_type::repair);
  REQUIRE(detail::get_packet_type(enc_handler.vec[3].data()) == detail::packet_type::repair);
  REQUIRE(detail::get_packet_type(enc_handler.vec[4].data()) == detail::packet_type::repair);

  SECTION("Lost first repair")
  {
    // Now send to decoder.
    // Lost first repair.
    dec(enc_handler.vec[1].data());
    dec(enc_handler.vec[2].data());
    dec(enc_handler.vec[3].data());
    dec(enc_handler.vec[4].data());

    REQUIRE(dec.nb_received_sources() == 0);
    REQUIRE(dec.nb_received_repairs() == 4);
    REQUIRE(dec.nb_missing_sources() == 0);
    REQUIRE(dec.nb_decoded() == 4);

    // All sources were correctly given to the user handler.
    REQUIRE(dec_data_handler.vec.size() == 4);
    REQUIRE(std::equal(begin(s0), end(s0), begin(dec_data_handler.vec[0])));
    REQUIRE(std::equal(begin(s1), end(s1), begin(dec_data_handler.vec[1])));
    REQUIRE(std::equal(begin(s2), end(s2), begin(dec_data_handler.vec[2])));
    REQUIRE(std::equal(begin(s3), end(s3), begin(dec_data_handler.vec[3])));
  }
  SECTION("Lost a repair")
  {
    // Now send to decoder.
    // Lost repair 2.
    dec(enc_handler.vec[0].data());
    dec(enc_handler.vec[1].data());
    dec(enc_handler.vec[3].data());
    dec(enc_handler.vec[4].data());

    REQUIRE(dec.nb_received_sources() == 0);
    REQUIRE(dec.nb_received_repairs() == 4);
    REQUIRE(dec.nb_missing_sources() == 0);
    REQUIRE(dec.nb_decoded() == 4);

    // All sources were correctly given to the user handler.
    REQUIRE(dec_data_handler.vec.size() == 4);
    REQUIRE(std::equal(begin(s0), end(s0), begin(dec_data_handler.vec[0])));
    REQUIRE(std::equal(begin(s1), end(s1), begin(dec_data_handler.vec[1])));
    REQUIRE(std::equal(begin(s2), end(s2), begin(dec_data_handler.vec[2])));
    REQUIRE(std::equal(begin(s3), end(s3), begin(dec_data_handler.vec[3])));
  }
  SECTION("Lost last repair")
  {
    // Now send to decoder.
    // Lost last repair.
    dec(enc_handler.vec[0].data());
    dec(enc_handler.vec[1].data());
    dec(enc_handler.vec[2].data());
    dec(enc_handler.vec[3].data());

    REQUIRE(dec.nb_received_sources() == 0);
    REQUIRE(dec.nb_received_repairs() == 4);
    REQUIRE(dec.nb_missing_sources() == 0);
    REQUIRE(dec.nb_decoded() == 4);

    // All sources were correctly given to the user handler.
    REQUIRE(dec_data_handler.vec.size() == 4);
    REQUIRE(std::equal(begin(s0), end(s0), begin(dec_data_handler.vec[0])));
    REQUIRE(std::equal(begin(s1), end(s1), begin(dec_data_handler.vec[1])));
    REQUIRE(std::equal(begin(s2), end(s2), begin(dec_data_handler.vec[2])));
    REQUIRE(std::equal(begin(s3), end(s3), begin(dec_data_handler.vec[3])));
  }
}

/*------------------------------------------------------------------------------------------------*/
