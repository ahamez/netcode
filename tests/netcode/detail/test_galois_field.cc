#include <array>
#include <algorithm>

#include "tests/catch.hpp"

#include "netcode/detail/galois_field.hh"

/*------------------------------------------------------------------------------------------------*/

using namespace ntc;

/*------------------------------------------------------------------------------------------------*/

TEST_CASE("compilate a galois_field", "[galois]")
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
//  {
//    detail::galois_field gf{16};
//    char a0[32] = {0,1,2,3,4,5,6,7
//                  ,0,1,2,3,4,5,6,7
//                  ,0,1,2,3,4,5,6,7
//                  ,0,1,2,3,4,5,6,7};
//    char a1[4];
//    gf.multiply(a0, a1, 4, 42);
//  }
//  {
//    detail::galois_field gf{32};
//    char a0[32] = {0,1,2,3,4,5,6,7
//                  ,0,1,2,3,4,5,6,7
//                  ,0,1,2,3,4,5,6,7
//                  ,0,1,2,3,4,5,6,7};
//    char a1[32];
//    gf.multiply(a0, a1, 32, 9999);
//  }
}

/*------------------------------------------------------------------------------------------------*/

TEST_CASE("", "[galois]")
{
  {
    detail::galois_field gf{8};
    const auto mult = gf.multiply(4, 5);
    REQUIRE(gf.divide(mult, 4) == 5);
    REQUIRE(gf.divide(mult, 5) == 4);
  }
  {
    detail::galois_field gf{16};
    const auto mult = gf.multiply(4, 5);
    REQUIRE(gf.divide(mult, 4) == 5);
    REQUIRE(gf.divide(mult, 5) == 4);
  }
  {
    detail::galois_field gf{32};
    const auto mult = gf.multiply(4, 5);
    REQUIRE(gf.divide(mult, 4) == 5);
    REQUIRE(gf.divide(mult, 5) == 4);
  }

}

/*------------------------------------------------------------------------------------------------*/
