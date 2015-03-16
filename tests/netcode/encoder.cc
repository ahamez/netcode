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

TEST_CASE("Encoder's window size", "[encoder]" )
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

TEST_CASE("Encoder generates repairs", "[encoder][repair]" )
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

TEST_CASE("Encoder erases sources when an ack is received", "[encoder][ack]" )
{
  ntc::encoder encoder{dummy_handler{}, code{8}, 5, code_type::systematic, protocol::simple};

  // First, add some sources.
  for (auto i = 0ul; i < 4; ++i)
  {
    auto sym = ntc::auto_symbol{512};
    encoder.commit(std::move(sym));
  }
  REQUIRE(encoder.window_size() == 4);

  // Then create an ack for some sources, with a wrong id, just to try.
  const auto ack = detail::ack{{0,2,9}};

  // Serialize the ack.
  char data[2048]; // will hold the bytes of the serialized ack.
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

  detail::handler_derived<handler> h{handler{data, 0}};
  detail::protocol::simple serializer{h};
  serializer.write_ack(ack);

  // Finally, notify the encoder.
  encoder.notify(data);

  // The number of sources should have decreased.
  REQUIRE(encoder.window_size() == 2);
}

/*------------------------------------------------------------------------------------------------*/
