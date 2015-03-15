#include <algorithm>
#include <array>

#include "tests/catch.hpp"

#include "netcode/symbol.hh"

/*------------------------------------------------------------------------------------------------*/

using namespace ntc;

/*------------------------------------------------------------------------------------------------*/

TEST_CASE("manual symbol", "[symbol]" )
{
  std::array<char, 5> tab = {'a', 'b', 'c', 'd', 'e'};

  auto s = symbol{128};
  std::copy_n(begin(tab), 5, s.buffer());
  s.set_nb_written_bytes(5);
  REQUIRE(std::equal(s.buffer(), s.buffer() + 5, begin(tab)));
  s.resize_buffer(1024);
  REQUIRE(std::equal(s.buffer(), s.buffer() + 5, begin(tab)));
}

/*------------------------------------------------------------------------------------------------*/

TEST_CASE("auto symbol", "[symbol]" )
{
  std::array<char, 5> tab = {'a', 'b', 'c', 'd', 'e'};

  auto s = auto_symbol{};
  std::copy_n(begin(tab), 4, s.back_inserter());
}

/*------------------------------------------------------------------------------------------------*/

TEST_CASE("copy symbol", "[symbol]" )
{
  std::array<char, 5> tab = {'a', 'b', 'c', 'd', 'e'};
  auto s = copy_symbol(3, tab.data());
}

/*------------------------------------------------------------------------------------------------*/
