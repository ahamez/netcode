#include <algorithm>

#include "tests/catch.hpp"

#include "netcode/galois/field.hh"
#include "netcode/code.hh"
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
};

} // namespace unnamed

/*------------------------------------------------------------------------------------------------*/

TEST_CASE("Encoder's window size", "[encoder]")
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

TEST_CASE("Encoder generates repairs", "[encoder][repair]")
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

TEST_CASE("Encoder correctly handles new incoming packets", "[encoder]")
{
  ntc::encoder encoder{dummy_handler{}, code{8}, 5, code_type::systematic, protocol::simple};

  // First, add some sources.
  for (auto i = 0ul; i < 4; ++i)
  {
    auto sym = ntc::auto_symbol{512};
    encoder.commit(std::move(sym));
  }
  REQUIRE(encoder.window_size() == 4);

  // Will hold the bytes of the serialized ack.
  char data[2048];
  struct handler
  {
    char* data;
    std::size_t written;

    void
    on_ready_data(std::size_t len, const char* src)
    {
      std::copy_n(src, len, data + written);
      written += len;
    }
  };

  // Directly use the serializer that would have been called by the sender.
  detail::handler_derived<handler> h{handler{data, 0}};
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

  SECTION("incoming  repair")
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
