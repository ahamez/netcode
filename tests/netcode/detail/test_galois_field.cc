#include <array>
#include <algorithm>

#include "tests/catch.hpp"

#include "netcode/detail/galois_field.hh"

/*------------------------------------------------------------------------------------------------*/

using namespace ntc;

/*------------------------------------------------------------------------------------------------*/

TEST_CASE("compile a galois_field", "[galois]")
{
  {
    detail::galois_field gf{8};
    std::array<char, 32> a0 = {{0,1,2,3,4,5,6,7
                               ,0,1,2,3,4,5,6,7
                               ,0,1,2,3,4,5,6,7
                               ,0,1,2,3,4,5,6,7}};
    std::array<char, 32> a1;
    std::array<char, 32> a2;
    gf.multiply(a0.data(), a1.data(), 32, 42);
  }
}

/*------------------------------------------------------------------------------------------------*/

TEST_CASE("", "[galois]")
{
  SECTION("(x*y/x == y) and (x*y/y == x)")
  {
    detail::galois_field gf{8};
    const auto mult = gf.multiply(4, 5);
    REQUIRE(gf.divide(mult, 4) == 5);
    REQUIRE(gf.divide(mult, 5) == 4);
  }
  SECTION("(x*y/x == y) and (x*y/y == x)")
  {
    detail::galois_field gf{16};
    const auto mult = gf.multiply(4, 5);
    REQUIRE(gf.divide(mult, 4) == 5);
    REQUIRE(gf.divide(mult, 5) == 4);
  }
  SECTION("(x*y/x == y) and (x*y/y == x)")
  {
    detail::galois_field gf{32};
    const auto mult = gf.multiply(4, 5);
    REQUIRE(gf.divide(mult, 4) == 5);
    REQUIRE(gf.divide(mult, 5) == 4);
  }
  SECTION("x * 1/x == 1")
  {
    detail::galois_field gf{8};
    const auto inv = gf.divide(1, 5);
    REQUIRE(gf.multiply(inv, 5) == 1);
  }
}

/*------------------------------------------------------------------------------------------------*/
