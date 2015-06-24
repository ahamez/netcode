#include <array>
#include <algorithm>

#include "tests/catch.hpp"
#include "tests/netcode/launch.hh"

#include "netcode/detail/galois_field.hh"

/*------------------------------------------------------------------------------------------------*/

using namespace ntc;

/*------------------------------------------------------------------------------------------------*/

TEST_CASE("compiling a galois_field")
{
  launch([](std::uint8_t gf_size)
  {
    detail::galois_field gf{gf_size};
    std::array<char, 32> a0 = {{0,1,2,3,4,5,6,7
                               ,0,1,2,3,4,5,6,7
                               ,0,1,2,3,4,5,6,7
                               ,0,1,2,3,4,5,6,7}};
    std::array<char, 32> a1;
    gf.multiply(a0.data(), a1.data(), 32, 42);
  });
}

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
