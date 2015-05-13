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

TEST_CASE("Decoder generate acks when N packets are received")
{
  configuration conf;
  conf.rate = 100;
  conf.ack_frequency = std::chrono::milliseconds{0};
  conf.ack_nb_packets = 4;

  encoder<packet_handler> enc{packet_handler{}, conf};
  decoder<packet_handler, data_handler> dec{packet_handler{}, data_handler{}, conf};

  auto& enc_packet_handler = enc.packet_handler();
  auto& dec_packet_handler = dec.packet_handler();

  // Give a source to the encoder.
  const auto s0 = {'a', 'b', 'c'};
  enc(data{begin(s0), end(s0)});
  enc(data{begin(s0), end(s0)});
  enc(data{begin(s0), end(s0)});
  enc(data{begin(s0), end(s0)});
  REQUIRE(enc.window() == 4);
  REQUIRE(enc_packet_handler.nb_packets() == 4);
  REQUIRE(detail::get_packet_type(enc_packet_handler.vec[0].data()) == detail::packet_type::source);
  REQUIRE(detail::get_packet_type(enc_packet_handler.vec[1].data()) == detail::packet_type::source);
  REQUIRE(detail::get_packet_type(enc_packet_handler.vec[2].data()) == detail::packet_type::source);
  REQUIRE(detail::get_packet_type(enc_packet_handler.vec[3].data()) == detail::packet_type::source);

  // Send sources to decoder.
  dec(enc_packet_handler.vec[0].data());
  dec(enc_packet_handler.vec[1].data());
  dec(enc_packet_handler.vec[2].data());
  dec(enc_packet_handler.vec[3].data());

  REQUIRE(dec_packet_handler.nb_packets() == 1);
  REQUIRE(detail::get_packet_type(dec_packet_handler.vec[0].data()) == detail::packet_type::ack);
}

/*------------------------------------------------------------------------------------------------*/

void
test_case_0(bool in_order)
{
  configuration conf;
  conf.in_order = in_order;
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


TEST_CASE("In order decoder: lost packet with an encoder's limited window")
{
  test_case_0(true);
}

TEST_CASE("Out of order decoder: lost packet with an encoder's limited window")
{
  test_case_0(false);
}

/*------------------------------------------------------------------------------------------------*/

void
test_case_1(bool in_order)
{
  configuration conf;
  conf.in_order = in_order;
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

TEST_CASE("In order decoder: non systematic code")
{
  test_case_1(true);
}

TEST_CASE("Out of order decoder: non systematic code")
{
  test_case_1(false);
}

/*------------------------------------------------------------------------------------------------*/

TEST_CASE("Decoder invalid read scenario")
{
  configuration conf;
  conf.rate = 3;
  conf.code_type = code::non_systematic;
  conf.ack_frequency = std::chrono::milliseconds{0};

  encoder<packet_handler> enc{packet_handler{}, conf};
  decoder<packet_handler, data_handler> dec{packet_handler{}, data_handler{}, conf};

  auto& enc_handler = enc.packet_handler();
  auto& dec_data_handler = dec.data_handler();

  // Packets will be stored in enc_handler.vec.
  const auto s0 = {'a'};
  enc(data{begin(s0), end(s0)});

  const auto s1 = {'b'};
  enc(data{begin(s1), end(s1)});

  const auto s2 = {'c', 'c'};
  enc(data{begin(s2), end(s2)});

  // Only repairs
  REQUIRE(enc_handler.nb_packets() == 4 /* repairs */);
  REQUIRE(detail::get_packet_type(enc_handler.vec[0].data()) == detail::packet_type::repair);
  REQUIRE(detail::get_packet_type(enc_handler.vec[1].data()) == detail::packet_type::repair);
  REQUIRE(detail::get_packet_type(enc_handler.vec[2].data()) == detail::packet_type::repair);
  REQUIRE(detail::get_packet_type(enc_handler.vec[3].data()) == detail::packet_type::repair);

  SECTION("Lost 1st repair")
  {
    dec(enc_handler.vec[1].data());
    dec(enc_handler.vec[2].data());
    dec(enc_handler.vec[3].data());

    REQUIRE(dec.nb_received_sources() == 0);
    REQUIRE(dec.nb_received_repairs() == 3);
    REQUIRE(dec.nb_decoded() == 3);

    // All sources were correctly given to the user handler.
    REQUIRE(dec_data_handler.vec.size() == 3);
    REQUIRE(std::equal(begin(s0), end(s0), begin(dec_data_handler.vec[0])));
    REQUIRE(std::equal(begin(s1), end(s1), begin(dec_data_handler.vec[1])));
    REQUIRE(std::equal(begin(s2), end(s2), begin(dec_data_handler.vec[2])));
  }
}

/*------------------------------------------------------------------------------------------------*/

TEST_CASE("In order decoder")
{
  configuration conf;
  conf.in_order = true;
  conf.rate = 4;
  conf.ack_frequency = std::chrono::milliseconds{0};

  encoder<packet_handler> enc{packet_handler{}, conf};
  decoder<packet_handler, data_handler> dec{packet_handler{}, data_handler{}, conf};

  auto& enc_handler = enc.packet_handler();
  auto& dec_data_handler = dec.data_handler();

  // Packets will be stored in enc_handler.vec.
  const auto s0 = {'a', 'b', 'c'};
  enc(data{begin(s0), end(s0)});
  const auto s1 = {'d', 'e', 'f'};
  enc(data{begin(s1), end(s1)});
  const auto s2 = {'g', 'h', 'i'};
  enc(data{begin(s2), end(s2)});
  const auto s3 = {'j', 'k', 'l'};
  enc(data{begin(s3), end(s3)});

  REQUIRE(enc_handler.nb_packets() == 5 /* 4 src + 1 repair */);
  REQUIRE(detail::get_packet_type(enc_handler.vec[0].data()) == detail::packet_type::source);
  REQUIRE(detail::get_packet_type(enc_handler.vec[1].data()) == detail::packet_type::source);
  REQUIRE(detail::get_packet_type(enc_handler.vec[2].data()) == detail::packet_type::source);
  REQUIRE(detail::get_packet_type(enc_handler.vec[3].data()) == detail::packet_type::source);
  REQUIRE(detail::get_packet_type(enc_handler.vec[4].data()) == detail::packet_type::repair);

  SECTION("Wrong order of sources")
  {
    // Now send to decoder in wrong order.
    dec(enc_handler.vec[0].data());
    dec(enc_handler.vec[3].data());
    dec(enc_handler.vec[1].data());
    dec(enc_handler.vec[2].data());
    dec(enc_handler.vec[4].data());
    REQUIRE(dec.nb_received_sources() == 4);
    REQUIRE(dec.nb_received_repairs() == 1);
    REQUIRE(dec.nb_missing_sources() == 0);
    REQUIRE(dec.nb_decoded() == 0);

    // Sources were given to in the correct order the user handler.
    REQUIRE(dec_data_handler.vec.size() == 4);
    REQUIRE(std::equal(begin(s0), end(s0), begin(dec_data_handler.vec[0])));
    REQUIRE(std::equal(begin(s1), end(s1), begin(dec_data_handler.vec[1])));
    REQUIRE(std::equal(begin(s2), end(s2), begin(dec_data_handler.vec[2])));
    REQUIRE(std::equal(begin(s3), end(s3), begin(dec_data_handler.vec[3])));
  }

  SECTION("Reverse order of sources")
  {
    // Now send to decoder in wrong order.
    dec(enc_handler.vec[3].data());
    dec(enc_handler.vec[2].data());
    dec(enc_handler.vec[1].data());
    dec(enc_handler.vec[0].data());
    dec(enc_handler.vec[4].data()); // << repair
    REQUIRE(dec.nb_received_sources() == 4);
    REQUIRE(dec.nb_received_repairs() == 1);
    REQUIRE(dec.nb_missing_sources() == 0);
    REQUIRE(dec.nb_decoded() == 0);

    // Sources were given to in the correct order the user handler.
    REQUIRE(dec_data_handler.vec.size() == 4);
    REQUIRE(std::equal(begin(s0), end(s0), begin(dec_data_handler.vec[0])));
    REQUIRE(std::equal(begin(s1), end(s1), begin(dec_data_handler.vec[1])));
    REQUIRE(std::equal(begin(s2), end(s2), begin(dec_data_handler.vec[2])));
    REQUIRE(std::equal(begin(s3), end(s3), begin(dec_data_handler.vec[3])));
  }

  SECTION("Repair in middle")
  {
    dec(enc_handler.vec[0].data()); // s0
    dec(enc_handler.vec[4].data()); // << repair
    dec(enc_handler.vec[3].data()); // s3
    dec(enc_handler.vec[1].data()); // s1
    dec(enc_handler.vec[2].data()); // s2, will be reconstructed
    REQUIRE(dec.nb_received_sources() == 4);
    REQUIRE(dec.nb_received_repairs() == 1);
    REQUIRE(dec.nb_missing_sources() == 0);
    REQUIRE(dec.nb_decoded() == 1); //

    // Sources were given in the correct order the user handler.
    REQUIRE(dec_data_handler.vec.size() == 4);
    REQUIRE(std::equal(begin(s0), end(s0), begin(dec_data_handler.vec[0])));
    REQUIRE(std::equal(begin(s1), end(s1), begin(dec_data_handler.vec[1])));
    REQUIRE(std::equal(begin(s2), end(s2), begin(dec_data_handler.vec[2])));
    REQUIRE(std::equal(begin(s3), end(s3), begin(dec_data_handler.vec[3])));
  }
}

/*------------------------------------------------------------------------------------------------*/

TEST_CASE("In order decoder, missing sources")
{
  configuration conf;
  conf.in_order = true;
  conf.window = 3;
  conf.rate = 3;
  conf.ack_frequency = std::chrono::milliseconds{0};

  encoder<packet_handler> enc{packet_handler{}, conf};
  decoder<packet_handler, data_handler> dec{packet_handler{}, data_handler{}, conf};

  auto& enc_handler = enc.packet_handler();
  auto& dec_data_handler = dec.data_handler();

  // Packets will be stored in enc_handler.vec.
  const auto s0 = {'a', 'b', 'c'};
  enc(data{begin(s0), end(s0)});
  const auto s1 = {'d', 'e', 'f'};
  enc(data{begin(s1), end(s1)});
  const auto s2 = {'g', 'h', 'i'};
  enc(data{begin(s2), end(s2)});
  const auto s3 = {'j', 'k', 'l'};
  enc(data{begin(s3), end(s3)});
  const auto s4 = {'m', 'n'};
  enc(data{begin(s4), end(s4)});
  const auto s5 = {'o'};
  enc(data{begin(s5), end(s5)});

  REQUIRE(enc_handler.nb_packets() == 8 /* 6 src + 2 repair */);

  REQUIRE(detail::get_packet_type(enc_handler.vec[0].data()) == detail::packet_type::source);
  REQUIRE(detail::get_packet_type(enc_handler.vec[1].data()) == detail::packet_type::source);
  REQUIRE(detail::get_packet_type(enc_handler.vec[2].data()) == detail::packet_type::source);
  REQUIRE(detail::get_packet_type(enc_handler.vec[3].data()) == detail::packet_type::repair);

  REQUIRE(detail::get_packet_type(enc_handler.vec[4].data()) == detail::packet_type::source);
  REQUIRE(detail::get_packet_type(enc_handler.vec[5].data()) == detail::packet_type::source);
  REQUIRE(detail::get_packet_type(enc_handler.vec[6].data()) == detail::packet_type::source);
  REQUIRE(detail::get_packet_type(enc_handler.vec[7].data()) == detail::packet_type::repair);

  SECTION("Right order")
  {
    // s1 and s2 are lost, unable to reconstruct them.
    dec(enc_handler.vec[0].data());
    dec(enc_handler.vec[3].data()); // << repair
    REQUIRE(dec.nb_missing_sources() == 2);

    dec(enc_handler.vec[4].data());
    dec(enc_handler.vec[5].data());
    dec(enc_handler.vec[6].data());
    dec(enc_handler.vec[7].data()); // << repair, outdating sources 0, 1 and 2

    REQUIRE(dec.nb_received_sources() == 4);
    REQUIRE(dec.nb_received_repairs() == 2);
    REQUIRE(dec.nb_missing_sources() == 0);
    REQUIRE(dec.nb_decoded() == 0);

  // Sources were given in the correct order the user handler.
  REQUIRE(dec_data_handler.vec.size() == 4);
  REQUIRE(std::equal(begin(s0), end(s0), begin(dec_data_handler.vec[0])));
  REQUIRE(std::equal(begin(s3), end(s3), begin(dec_data_handler.vec[1])));
  REQUIRE(std::equal(begin(s4), end(s4), begin(dec_data_handler.vec[2])));
  REQUIRE(std::equal(begin(s5), end(s5), begin(dec_data_handler.vec[3])));
  }

  SECTION("Wrong order 1")
  {
    // s1 and s2 are lost, unable to reconstruct them.
    dec(enc_handler.vec[0].data()); // s0
    dec(enc_handler.vec[4].data()); // s3
    dec(enc_handler.vec[5].data()); // s4
    dec(enc_handler.vec[6].data()); // s5
    dec(enc_handler.vec[7].data()); // << repair for 3, 4 and 5; outdating sources 0, 1 and 2
    dec(enc_handler.vec[3].data()); // << repair for 0, 1 and 2

    REQUIRE(dec.nb_received_sources() == 4);
    REQUIRE(dec.nb_received_repairs() == 2);
    REQUIRE(dec.nb_missing_sources() == 0);
    REQUIRE(dec.nb_decoded() == 0);

    // Sources were given in the correct order the user handler.
    REQUIRE(dec_data_handler.vec.size() == 4);
    REQUIRE(std::equal(begin(s0), end(s0), begin(dec_data_handler.vec[0])));
    REQUIRE(std::equal(begin(s3), end(s3), begin(dec_data_handler.vec[1])));
    REQUIRE(std::equal(begin(s4), end(s4), begin(dec_data_handler.vec[2])));
    REQUIRE(std::equal(begin(s5), end(s5), begin(dec_data_handler.vec[3])));
  }

  SECTION("Wrong order 2")
  {
    // s1 and s2 are lost, unable to reconstruct them.
    dec(enc_handler.vec[4].data()); // s3
    dec(enc_handler.vec[5].data()); // s4
    dec(enc_handler.vec[6].data()); // s5
    dec(enc_handler.vec[7].data()); // << repair for 3, 4 and 5; outdating sources 0, 1 and 2
    dec(enc_handler.vec[3].data()); // << repair for 0, 1 and 2
    dec(enc_handler.vec[0].data()); // s0

    REQUIRE(dec.nb_received_sources() == 4);
    REQUIRE(dec.nb_received_repairs() == 2);
    REQUIRE(dec.nb_missing_sources() == 0);
    REQUIRE(dec.nb_decoded() == 0);

    // Sources were given in the correct order the user handler.
    REQUIRE(dec_data_handler.vec.size() == 3);
    REQUIRE(std::equal(begin(s3), end(s3), begin(dec_data_handler.vec[0])));
    REQUIRE(std::equal(begin(s4), end(s4), begin(dec_data_handler.vec[1])));
    REQUIRE(std::equal(begin(s5), end(s5), begin(dec_data_handler.vec[2])));
  }

  SECTION("Wrong order 3")
  {
    // s1 and s2 are lost, unable to reconstruct them.
    dec(enc_handler.vec[5].data()); // s4
    dec(enc_handler.vec[6].data()); // s5
    dec(enc_handler.vec[7].data()); // << repair for 3, 4 and 5; outdating sources 0, 1 and 2
    dec(enc_handler.vec[3].data()); // << repair for 0, 1 and 2
    dec(enc_handler.vec[0].data()); // s0
    dec(enc_handler.vec[4].data()); // s3

    REQUIRE(dec.nb_received_sources() == 4);
    REQUIRE(dec.nb_received_repairs() == 2);
    REQUIRE(dec.nb_missing_sources() == 0);
    REQUIRE(dec.nb_decoded() == 1);

    // Sources were given in the correct order the user handler.
    REQUIRE(dec_data_handler.vec.size() == 3);
    REQUIRE(std::equal(begin(s3), end(s3), begin(dec_data_handler.vec[0])));
    REQUIRE(std::equal(begin(s4), end(s4), begin(dec_data_handler.vec[1])));
    REQUIRE(std::equal(begin(s5), end(s5), begin(dec_data_handler.vec[2])));
  }
}

/*------------------------------------------------------------------------------------------------*/
