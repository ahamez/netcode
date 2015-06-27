#include <array>
#include <algorithm>

#include "tests/catch.hpp"
#include "tests/netcode/launch.hh"

#include "netcode/detail/galois_field.hh"

/*------------------------------------------------------------------------------------------------*/

using namespace ntc;

/*------------------------------------------------------------------------------------------------*/

TEST_CASE("Verify operations on single elements")
{
  launch([](std::uint8_t gf_size)
  {
    SECTION("(1/x * x == 1)")
    {
      detail::galois_field gf{gf_size};
      const auto inv = gf.invert(5);
      REQUIRE(gf.multiply(5, inv) == 1);
    }
  });
}

/*------------------------------------------------------------------------------------------------*/
