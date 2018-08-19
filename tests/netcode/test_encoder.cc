#include <algorithm>

#include <catch.hpp>
#include "tests/netcode/common.hh"
#include "tests/netcode/launch.hh"

#include "netcode/encoder.hh"

/*------------------------------------------------------------------------------------------------*/

using namespace ntc;

/*------------------------------------------------------------------------------------------------*/

TEST_CASE("Encoder's window size")
{
  launch([](std::uint8_t gf_size)
  {
    const auto data = std::vector<char>(100, 'x');

    encoder<packet_handler> encoder{gf_size, packet_handler{}};
    encoder.set_rate(1);

    auto data0 = ntc::data(data.begin(), data.begin() + 12);
    encoder(std::move(data0));
    REQUIRE(encoder.window() == 1);

    auto data1 = ntc::data(data.begin(), data.begin() + 16);
    encoder(std::move(data1));
    REQUIRE(encoder.window() == 2);

    auto data2 = ntc::data(data.begin(), data.begin() + 64);
    encoder(std::move(data2));
    REQUIRE(encoder.window() == 3);
  });
}

/*------------------------------------------------------------------------------------------------*/

TEST_CASE("Encoder can limit the window size")
{
  launch([](std::uint8_t gf_size)
  {
    const auto data = std::vector<char>(100, 'x');

    encoder<packet_handler> encoder{gf_size, packet_handler{}};
    encoder.set_window_size(4);

    auto data0 = ntc::data(data.begin(), data.begin() + 8);
    encoder(std::move(data0));

    data0 = ntc::data(data.begin(), data.begin() + 16);
    encoder(std::move(data0));

    data0 = ntc::data(data.begin(), data.begin() + 32);
    encoder(std::move(data0));

    data0 = ntc::data(data.begin(), data.begin() + 8);
    encoder(std::move(data0));

    data0 = ntc::data(data.begin(), data.begin() + 24);
    encoder(std::move(data0));
    REQUIRE(encoder.window() == 4);

    data0 = ntc::data(data.begin(), data.begin() + 72);
    encoder(std::move(data0));
    REQUIRE(encoder.window() == 4);
  });
}

/*------------------------------------------------------------------------------------------------*/

TEST_CASE("Encoder generates repairs")
{
  launch([](std::uint8_t gf_size)
  {
    const auto data = std::vector<char>(100, 'x');

    encoder<packet_handler> encoder{gf_size, packet_handler{}};
    encoder.set_rate(5);

    for (auto i = 0ul; i < 100; ++i)
    {
      encoder(ntc::data(data.begin(), data.begin() + 96));
    }
    REQUIRE(encoder.nb_sent_repairs() == (100/5 /*code rate*/));
  });
}

/*------------------------------------------------------------------------------------------------*/

TEST_CASE("Encoder correctly handles new incoming packets")
{
  launch([](std::uint8_t gf_size)
  {
    encoder<packet_handler> encoder{gf_size, packet_handler{}};
    encoder.set_rate(5);

    // First, add some sources.
    for (auto i = 0ul; i < 4; ++i)
    {
      encoder(ntc::data(512));
    }
    REQUIRE(encoder.window() == 4);

    struct handler
    {
      packet pkt;

      void
      operator()(const char* src, std::size_t len)
      {
        std::copy_n(src, len, std::back_inserter(pkt));
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
      const auto result = encoder(packet{h.pkt});
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
      const auto result0 = encoder(packet{h.pkt});
      REQUIRE(result0);

      // The number of sources should have decreased.
      REQUIRE(encoder.window() == 2);

      // Reset handler.
      h.pkt.clear();

      // Create an ack for some sources, with an already deleted source.
      const auto ack1 = detail::ack{{0}, 0};

      // Serialize the ack.
      serializer.write_ack(ack1);

      // Finally, notify the encoder.
      const auto result1 = encoder(packet{h.pkt});
      REQUIRE(result1);

      // The number of sources should have decreased.
      REQUIRE(encoder.window() == 2);

      // Reset handler.
      h.pkt.clear();

      // Create an ack for some sources, with a source that wasn't deleted before.
      const auto ack2 = detail::ack{{1}, 0};

      // Serialize the ack.
      serializer.write_ack(ack2);

      // Finally, notify the encoder.
      const auto result2 = encoder(packet{h.pkt});
      REQUIRE(result2);

      // The number of sources should have decreased.
      REQUIRE(encoder.window() == 1);
    }

    SECTION("incoming repair")
    {
      // Create a repair.
      const auto repair = detail::encoder_repair{0, 42, {0,1}, detail::zero_byte_buffer{'x'}};

      // Serialize the repair.
      serializer.write_repair(repair);

      // Finally, notify the encoder.
      REQUIRE_THROWS_AS(encoder(packet{h.pkt}), ntc::packet_type_error);

      // The number of sources should not have decreased.
      REQUIRE(encoder.window() == 4);
    }

    SECTION("incoming source")
    {
      // Create a source.
      const auto source = detail::encoder_source{0, {}};
      
      // Serialize the source.
      serializer.write_source(source);
      
      // Finally, notify the encoder.
      REQUIRE_THROWS_AS(encoder(packet{h.pkt}), ntc::packet_type_error);
      
      // The number of sources should not have decreased.
      REQUIRE(encoder.window() == 4);
    }
  });
}

/*------------------------------------------------------------------------------------------------*/

TEST_CASE("Encoder sends correct sources")
{
  launch([](std::uint8_t gf_size)
  {
    encoder<packet_handler> enc{gf_size, packet_handler{}};

    auto& enc_handler = enc.packet_handler();

    const auto s0 = {'A','B','C','D'};

    enc(data{begin(s0), end(s0)});
    REQUIRE(enc_handler[0].size() == ( sizeof(std::uint8_t)      // type
                                     + sizeof(std::uint32_t)     // id
                                     + sizeof(std::uint16_t)     // user data size
                                     + s0.size()                 // data
                                     ));
    REQUIRE(std::equal( begin(s0), end(s0)
                      , enc_handler[0].begin() + sizeof(std::uint8_t) + sizeof(std::uint32_t)
                                               + sizeof(std::uint16_t)
                      ));
  });
}

/*------------------------------------------------------------------------------------------------*/

TEST_CASE("Encoder sends repairs")
{
  launch([](std::uint8_t gf_size)
  {
    encoder<packet_handler> enc{gf_size, packet_handler{}};
    enc.set_rate(1);

    auto& enc_handler = enc.packet_handler();

    const auto s0 = {'a','b','c','d'};

    enc(data{begin(s0), end(s0)});
    REQUIRE(enc_handler.nb_packets() == 2 /* 1 source +  1 repair */);
    REQUIRE(detail::get_packet_type(enc_handler[0]) == detail::packet_type::source);
    REQUIRE(detail::get_packet_type(enc_handler[1]) == detail::packet_type::repair);
  });
}

/*------------------------------------------------------------------------------------------------*/

TEST_CASE("Decoder: invalid memory access scenario")
{
  launch([](std::uint8_t gf_size)
  {
    // A bug occurred when the pre-allocated size of a data was larger than the size of the cached
    // repair in the encoder.
    static constexpr auto max_len = 4096;

    encoder<packet_handler> enc{gf_size, packet_handler{}};
    enc.set_rate(5);

    auto& enc_handler = enc.packet_handler();

    // Packets will be stored in enc_handler.vec.
    const std::vector<char> x = {'x','x','x','x'};
    const std::vector<char> y = {'y','y','y','y'};

    {
      auto data_ptr = std::make_shared<ntc::data>(max_len);
      std::copy(x.begin(),  x.end(), data_ptr->begin());
      data_ptr->resize(x.size());
      enc(std::move(*data_ptr));
    }
    {
      auto data_ptr = std::make_shared<ntc::data>(max_len);
      std::copy(y.begin(),  y.end(), data_ptr->begin());
      data_ptr->resize(y.size());
      enc(std::move(*data_ptr));
    }
    {
      auto data_ptr = std::make_shared<ntc::data>(max_len);
      std::copy(y.begin(),  y.end(), data_ptr->begin());
      data_ptr->resize(y.size());
      enc(std::move(*data_ptr));
    }
    {
      auto data_ptr = std::make_shared<ntc::data>(max_len);
      std::copy(y.begin(),  y.end(), data_ptr->begin());
      data_ptr->resize(y.size());
      enc(std::move(*data_ptr));
    }
    {
      auto data_ptr = std::make_shared<ntc::data>(max_len);
      std::copy(y.begin(),  y.end(), data_ptr->begin());
      data_ptr->resize(y.size());
      enc(std::move(*data_ptr));
    }
    REQUIRE(enc_handler.nb_packets() == 6);
    REQUIRE(enc.nb_sent_repairs() == 1);
    REQUIRE(enc.nb_sent_sources() == 5);
    REQUIRE(enc.nb_received_acks() == 0);
  });
}

/*------------------------------------------------------------------------------------------------*/

TEST_CASE("Non systematic encoder")
{
  launch([](std::uint8_t gf_size)
  {
    encoder<packet_handler> encoder{gf_size, packet_handler{}};
    encoder.set_rate(3);
    encoder.set_code_type(systematic::no);
    auto& enc_handler = encoder.packet_handler();

    const auto data = std::vector<char>(100, 'x');

    auto data0 = ntc::data(data.begin(), data.begin() + 8);
    encoder(std::move(data0));
    REQUIRE(encoder.nb_sent_sources() == 0);
    REQUIRE(encoder.window() == 1);
    REQUIRE(encoder.nb_sent_repairs() == 1);

    data0 = ntc::data(data.begin(), data.begin() + 24);
    encoder(std::move(data0));
    REQUIRE(encoder.nb_sent_sources() == 0);
    REQUIRE(encoder.window() == 2);
    REQUIRE(encoder.nb_sent_repairs() == 2);

    data0 = ntc::data(data.begin(), data.begin() + 76);
    encoder(std::move(data0));
    REQUIRE(encoder.nb_sent_sources() == 0);
    REQUIRE(encoder.window() == 3);
    REQUIRE(encoder.nb_sent_repairs() == 4);

    data0 = ntc::data(data.begin(), data.begin() + 32);
    encoder(std::move(data0));
    REQUIRE(encoder.nb_sent_sources() == 0);
    REQUIRE(encoder.window() == 4);
    REQUIRE(encoder.nb_sent_repairs() == 5);

    REQUIRE(enc_handler.nb_packets() == 5 /* 5 repairs */);
    REQUIRE(detail::get_packet_type(enc_handler[0]) == detail::packet_type::repair);
    REQUIRE(detail::get_packet_type(enc_handler[1]) == detail::packet_type::repair);
    REQUIRE(detail::get_packet_type(enc_handler[2]) == detail::packet_type::repair);
    REQUIRE(detail::get_packet_type(enc_handler[3]) == detail::packet_type::repair);
    REQUIRE(detail::get_packet_type(enc_handler[4]) == detail::packet_type::repair);
  });
}

/*------------------------------------------------------------------------------------------------*/

TEST_CASE("Encoder rejects sources and repairs")
{
  launch([](std::uint8_t gf_size)
  {
    encoder<packet_handler> encoder{gf_size, packet_handler{}};
    packet_handler h;
    detail::packetizer<packet_handler> serializer{h};

    SECTION("repair")
    {
      serializer.write_repair( detail::encoder_repair{42, 54, {0,1}
                             , detail::zero_byte_buffer(1024, 'x')});
      REQUIRE_THROWS_AS(encoder(h[0]), packet_type_error);
    }

    SECTION("source")
    {
      serializer.write_source(detail::encoder_source{394839, {'a', 'b', 'c', 'd'}});
      REQUIRE_THROWS_AS(encoder(h[0]), packet_type_error);
    }

    SECTION("Garbage")
    {
      REQUIRE_THROWS_AS(encoder(packet{33,35,1,0}), packet_type_error);
    }
  });
}

/*------------------------------------------------------------------------------------------------*/

TEST_CASE("Encoder adapts rate automatically")
{
  launch([](std::uint8_t gf_size)
  {
    encoder<packet_handler> enc{gf_size, packet_handler{}};
    enc.set_rate(1);
    enc.set_adaptive(true);

    // Simulate decoder.
    packet_handler h_decoder;
    detail::packetizer<packet_handler> serializer{h_decoder};

    const auto d = std::vector<char>(32, 'x');
    for (auto i = 0ul; i < 100; ++i)
    {
      enc(data(d.begin(), d.end()));
    }
    REQUIRE(enc.window() == 100);
    REQUIRE(enc.rate() == 1);

    // An ack that indicates that no packets were lost.
    auto ids0 = detail::source_id_list{};
    for (auto i = 0u; i < 100; ++i)
    {
      ids0.insert(i);
    }
    serializer.write_ack(detail::ack{std::move(ids0), 200});

    REQUIRE_NOTHROW(enc(packet{h_decoder[0]}));
    REQUIRE(enc.window() == 0);
    REQUIRE(enc.rate() == 50); // default maximal rate

    for (auto i = 100ul; i < 200; ++i)
    {
      enc(data(d.begin(), d.end()));
    }
    REQUIRE(enc.window() == 100);

    // An ack that indicates that half of the packets were lost.
    auto ids1 = detail::source_id_list{};
    for (auto i = 0u; i < 50; ++i)
    {
      ids1.insert(100 + 2*i);
    }
    serializer.write_ack(detail::ack{std::move(ids1), 50});
    
    REQUIRE_NOTHROW(enc(packet{h_decoder[1]}));
    REQUIRE(enc.rate() == 1); // minimal rate
  });
}

/*------------------------------------------------------------------------------------------------*/
