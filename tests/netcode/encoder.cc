#include "tests/catch.hpp"

#include "netcode/galois/field.hh"
#include "netcode/coding.hh"
#include "netcode/encoder.hh"

/*------------------------------------------------------------------------------------------------*/

using namespace ntc;

/*------------------------------------------------------------------------------------------------*/

namespace /* unnamed */ {

struct handler
{
  void
  on_ready_data(std::size_t, const char*)
  {
  }
};

const auto dummy_generator = [](std::uint32_t x){return x;};

} // namespace unnamed

/*------------------------------------------------------------------------------------------------*/

TEST_CASE("Encoder's window size", "[encoder]" )
{
  ntc::encoder encoder{handler{}, {galois::field{8}, dummy_generator}, 3};

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

TEST_CASE("Encoder generate repairs", "[encoder]" )
{
  ntc::encoder encoder{handler{}, {galois::field{8}, dummy_generator}, 5};

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
