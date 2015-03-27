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
  on_ready_data(std::size_t, const char*)
  {}

  void
  on_ready_symbol(std::size_t, const char*)
  {}
};

} // namespace unnamed

/*------------------------------------------------------------------------------------------------*/

TEST_CASE("Encoder's window size")
{
  ntc::encoder encoder{dummy_handler{}, 3};

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
  ntc::encoder encoder{dummy_handler{}, 5, 4/* window */, code_type::systematic, protocol::simple};

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
  ntc::encoder encoder{dummy_handler{}, 5};

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
  ntc::encoder encoder{dummy_handler{}, 5 /* rate */, 100, code_type::systematic, protocol::simple};

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
  struct handler
  {
    char* data;
    std::size_t& written;

    void
    on_ready_data(std::size_t len, const char* src)
    {
      std::copy_n(src, len, data + written);
      written += len;
    }
    void
    on_ready_symbol(std::size_t, const char*)
    {}
  };

  // Directly use the serializer that would have been called by the sender.
  detail::handler_derived<handler> h{handler{data.buffer(), nb_written}};
  detail::protocol::simple serializer{h};

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
