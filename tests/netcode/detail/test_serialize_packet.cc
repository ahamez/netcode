#include <sstream>

#include <catch.hpp>

#include "netcode/detail/serialize_packet.hh"

/*------------------------------------------------------------------------------------------------*/

using namespace ntc;

/*------------------------------------------------------------------------------------------------*/

TEST_CASE("Packet serialization")
{
  packet p0{'a','b','c','d','e','f','g','h'};

  std::stringstream ss;
  detail::serialize_packet::write(ss, p0);

  const auto p1 = detail::serialize_packet::read(ss);

  REQUIRE(p0.size() == p1.size());
  REQUIRE(std::equal(p0.begin(), p0.end(), p1.begin()));
}

/*------------------------------------------------------------------------------------------------*/
