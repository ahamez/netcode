#include <algorithm>
#include <array>

#include "tests/catch.hpp"

#include "netcode/symbol.hh"

/*------------------------------------------------------------------------------------------------*/

using namespace ntc;

/*------------------------------------------------------------------------------------------------*/

TEST_CASE("symbol construction", "[symbol]" )
{
    std::array<char, 5> tab = {'a', 'b', 'c', 'd', 'e'};

  SECTION("Manual")
  {
    auto s = symbol{512};
    std::copy_n(begin(tab), 5, s.buffer());
    s.set_nb_written_bytes(5);
    REQUIRE(s.user_size() == 5);
    REQUIRE(std::equal(s.buffer(), s.buffer() + 5, begin(tab)));
  }

  SECTION("Auto")
  {
    auto s = auto_symbol{};
    std::copy_n(begin(tab), 4, s.back_inserter());
    // Can't really test the user size as it is set by encoder::commit().
    // REQUIRE(s.user_size() == 0);
    REQUIRE(std::equal(begin(s.buffer()), end(s.buffer()), begin(tab)));
  }

  SECTION("Copy")
  {
    std::array<char, 3> tab = {'a', 'b', 'c'};
    auto s = copy_symbol(3, tab.data());
    // The real buffer might be larger.
    REQUIRE(s.buffer_size() >= 3);
    REQUIRE(s.user_size() == 3);
    REQUIRE(std::equal(begin(tab), end(tab), begin(s.buffer())));
  }
}

/*------------------------------------------------------------------------------------------------*/
