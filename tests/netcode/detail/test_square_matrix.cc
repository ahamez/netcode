#include <algorithm>

#include "tests/catch.hpp"

#include "netcode/detail/square_matrix.hh"

/*------------------------------------------------------------------------------------------------*/

using namespace ntc;

/*------------------------------------------------------------------------------------------------*/

TEST_CASE("A square_matrix stores column by column", "[square_matrix]" )
{
  detail::square_matrix m{4};
  std::uint32_t k = 0u;
  for (auto i = 0ul; i < 4; ++i)
  {
    for (auto j = 0ul; j < 4; ++j, ++k)
    {
      m(i, j) = k;
    }
  }

                // r0 r1 r2  r3
  const auto l0 = {0, 4,  8, 12};
  const auto l1 = {1, 5,  9, 13};
  const auto l2 = {2, 6, 10, 14};
  const auto l3 = {3, 7, 11, 15};

  REQUIRE(std::equal(begin(l0), end(l0), m.column(0)));
  REQUIRE(std::equal(begin(l1), end(l1), m.column(1)));
  REQUIRE(std::equal(begin(l2), end(l2), m.column(2)));
  REQUIRE(std::equal(begin(l3), end(l3), m.column(3)));
}

/*------------------------------------------------------------------------------------------------*/
