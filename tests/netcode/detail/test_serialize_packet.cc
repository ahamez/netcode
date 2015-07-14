#include <sstream>

#include "tests/catch.hpp"

#include "netcode/detail/serialize_packet.hh"

/*------------------------------------------------------------------------------------------------*/

using namespace ntc;

/*------------------------------------------------------------------------------------------------*/

TEST_CASE("Compact")
{
  packet p0{'a','b','c','d','e','f','g','h'};

  std::stringstream ss;
  detail::serialize_packet::write(ss, p0, true);

  const auto p1 = detail::serialize_packet::read(ss, true);

  REQUIRE(p0.size() == p1.size());
  REQUIRE(std::equal(p0.begin(), p0.end(), p1.begin()));
}

/*------------------------------------------------------------------------------------------------*/

TEST_CASE("Not compact")
{
  packet p0{'a','b','c','d','e','f','g','h'};

  std::stringstream ss;
  detail::serialize_packet::write(ss, p0, false);

  const auto p1 = detail::serialize_packet::read(ss, false);

  REQUIRE(p0.size() == p1.size());
  REQUIRE(std::equal(p0.begin(), p0.end(), p1.begin()));
}

/*------------------------------------------------------------------------------------------------*/
