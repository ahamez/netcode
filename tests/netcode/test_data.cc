#include <algorithm>
#include <array>

#include "tests/catch.hpp"

#include "netcode/data.hh"

/*------------------------------------------------------------------------------------------------*/

using namespace ntc;

/*------------------------------------------------------------------------------------------------*/

TEST_CASE("manual data")
{
  std::array<char, 5> tab = {{'a', 'b', 'c', 'd', 'e'}};

  auto s = data{128};
  std::copy_n(begin(tab), 5, s.buffer());
  s.used_bytes() = 5;
  REQUIRE(std::equal(s.buffer(), s.buffer() + 5, begin(tab)));
  s.resize(1024);
  REQUIRE(std::equal(s.buffer(), s.buffer() + 5, begin(tab)));
}

/*------------------------------------------------------------------------------------------------*/

TEST_CASE("copy data")
{
  std::array<char, 5> tab = {{'a', 'b', 'c', 'd', 'e'}};
  auto s = copy_data{tab.data(), 3};
}

/*------------------------------------------------------------------------------------------------*/
