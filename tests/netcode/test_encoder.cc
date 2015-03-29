#include <algorithm>

#include "tests/catch.hpp"

#include "netcode/encoder.hh"

/*------------------------------------------------------------------------------------------------*/

using namespace ntc;

/*------------------------------------------------------------------------------------------------*/

namespace /* unnamed */ {

struct dummy_handler
{
  void
  operator()(const char*, std::size_t)
  {}
};

} // namespace unnamed

/*------------------------------------------------------------------------------------------------*/

TEST_CASE("Encoder's window size")
{
  default_configuration conf;
  conf.rate = 3;
  ntc::encoder encoder{dummy_handler{}, conf};

  SECTION("Ever growing window size")
  {
    for (auto i = 0ul; i < 100; ++i)
    {
      auto sym = ntc::symbol{512};
      sym.set_nb_written_bytes(300);
      encoder.commit(std::move(sym));
      REQUIRE(encoder.window_size() == (i + 1));
    }
  }
}

/*------------------------------------------------------------------------------------------------*/

TEST_CASE("Encoder can limit the window size")
{
  default_configuration conf;
  conf.window = 4;
  ntc::encoder encoder{dummy_handler{}, conf};

  auto sym = ntc::symbol{512};
  sym.set_nb_written_bytes(8);
  encoder.commit(std::move(sym));

  sym = ntc::symbol{512};
  sym.set_nb_written_bytes(8);
  encoder.commit(std::move(sym));

  sym = ntc::symbol{512};
  sym.set_nb_written_bytes(8);
  encoder.commit(std::move(sym));

  sym = ntc::symbol{512};
  sym.set_nb_written_bytes(8);
  encoder.commit(std::move(sym));

  sym = ntc::symbol{512};
  sym.set_nb_written_bytes(8);
  encoder.commit(std::move(sym));
  REQUIRE(encoder.window_size() == 4);

  sym = ntc::symbol{512};
  sym.set_nb_written_bytes(8);
  encoder.commit(std::move(sym));
  REQUIRE(encoder.window_size() == 4);
}

/*------------------------------------------------------------------------------------------------*/

TEST_CASE("Encoder generates repairs")
{
  default_configuration conf;
  conf.rate = 5;
  ntc::encoder encoder{dummy_handler{}, conf};

  SECTION("Fixed code rate")
  {
    for (auto i = 0ul; i < 100; ++i)
    {
      auto sym = ntc::symbol{512};
      sym.set_nb_written_bytes(512);
      encoder.commit(std::move(sym));
    }
    REQUIRE(encoder.nb_repairs() == (100/5 /*code rate*/));
  }
}

/*------------------------------------------------------------------------------------------------*/

TEST_CASE("Encoder correctly handles new incoming packets")
{
  default_configuration conf;
  conf.rate = 5;
  ntc::encoder encoder{dummy_handler{}, conf};

  // First, add some sources.
  for (auto i = 0ul; i < 4; ++i)
  {
    auto sym = ntc::auto_symbol{512};
    encoder.commit(std::move(sym));
  }
  REQUIRE(encoder.window_size() == 4);

  // Will hold the bytes of the serialized ack.
  packet data{2048};
  std::size_t nb_written = 0;
  struct my_handler
  {
    char* data;
    std::size_t& written;

    void
    operator()(const char* src, std::size_t len)
    {
      if (src)
      {
        std::copy_n(src, len, data + written);
        written += len;
      }
    }
  };

  // Directly use the serializer that would have been called by the sender.
  handler h = my_handler{data.buffer(), nb_written};
  detail::packetizer_simple serializer{h};

  SECTION("incoming ack")
  {
    // Create an ack for some sources, with a wrong id, just to try.
    const auto ack = detail::ack{{0,2,9}};

    // Serialize the ack.
    serializer.write_ack(ack);

    // Finally, notify the encoder.
    const auto result = encoder.notify(data);
    REQUIRE(result);

    // The number of sources should have decreased.
    REQUIRE(encoder.window_size() == 2);
  }

  SECTION("incoming ack 2")
  {
    // Create an ack for some sources, with a wrong id, just to try.
    const auto ack0 = detail::ack{{0,2,9}};

    // Serialize the ack.
    serializer.write_ack(ack0);

    // Finally, notify the encoder.
    const auto result0 = encoder.notify(data);
    REQUIRE(result0);

    // The number of sources should have decreased.
    REQUIRE(encoder.window_size() == 2);

    /// Reset handler.
    nb_written = 0;

    // Create an ack for some sources, with an already deleted source.
    const auto ack1 = detail::ack{{0}};

    // Serialize the ack.
    serializer.write_ack(ack1);

    // Finally, notify the encoder.
    const auto result1 = encoder.notify(data);
    REQUIRE(result1);

    // The number of sources should have decreased.
    REQUIRE(encoder.window_size() == 2);

    /// Reset handler.
    nb_written = 0;
    
    // Create an ack for some sources, with a source that wasn't deleted before.
    const auto ack2 = detail::ack{{1}};

    // Serialize the ack.
    serializer.write_ack(ack2);

    // Finally, notify the encoder.
    const auto result2 = encoder.notify(data);
    REQUIRE(result2);

    // The number of sources should have decreased.
    REQUIRE(encoder.window_size() == 1);
  }

  SECTION("incoming repair")
  {
    // Create a repair.
    const auto repair = detail::repair{0};

    // Serialize the repair.
    serializer.write_repair(repair);

    // Finally, notify the encoder.
    const auto result = encoder.notify(data);
    REQUIRE(not result);

    // The number of sources should not have decreased.
    REQUIRE(encoder.window_size() == 4);
  }

  SECTION("incoming source")
  {
    // Create a source.
    const auto source = detail::source{0, {}, 0};

    // Serialize the source.
    serializer.write_source(source);

    // Finally, notify the encoder.
    const auto result = encoder.notify(data);
    REQUIRE(not result);

    // The number of sources should not have decreased.
    REQUIRE(encoder.window_size() == 4);
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
    if (src)
    {
      std::copy_n(src, len, data + written);
      written += len;
    }
    else
    {
      nb_packets += 1;
    }
  }
};

} // namespace unnamed

/*------------------------------------------------------------------------------------------------*/

TEST_CASE("Encoder sends correct sources")
{
  ntc::encoder enc{my_handler{}};

  auto& enc_handler = *enc.data_handler().target<my_handler>();

  const auto s0 = {'A', 'B', 'C'};

  enc.commit(copy_symbol{begin(s0), end(s0)});
  // Test specific to packetizer_simple.
  REQUIRE(enc_handler.written == ( sizeof(std::uint8_t)      // type
                                 + sizeof(std::uint32_t)     // id
                                 + sizeof(std::uint16_t)     // user symbol size
                                 + s0.size()                 // symbol
                                 ));
  REQUIRE(std::equal( begin(s0), end(s0)
                    , enc_handler.data + sizeof(std::uint8_t) + sizeof(std::uint32_t)
                                       + sizeof(std::uint16_t)
                    ));

}

/*------------------------------------------------------------------------------------------------*/

TEST_CASE("Encoder sends repairs")
{
  default_configuration conf;
  conf.rate = 1; // A repair for a source

  ntc::encoder enc{my_handler{}, conf};

  auto& enc_handler = *enc.data_handler().target<my_handler>();

  const auto s0 = {'a', 'b', 'c'};

  enc.commit(copy_symbol{begin(s0), end(s0)});
  // Test specific to packetizer_simple.
  const auto src_sz = sizeof(std::uint8_t)      // type
                    + sizeof(std::uint32_t)     // id
                    + sizeof(std::uint16_t)     // user symbol size
                    + s0.size();                // symbol

  REQUIRE(enc_handler.nb_packets == 2 /* 1 source +  1 repair */);
  REQUIRE(detail::get_packet_type(enc_handler.data + src_sz) == detail::packet_type::repair);
}

/*------------------------------------------------------------------------------------------------*/
