#include <algorithm> // equal

#include "tests/catch.hpp"

#include "netcode/detail/decoder.hh"
#include "netcode/detail/encoder.hh"
#include "netcode/detail/source_list.hh"

/*------------------------------------------------------------------------------------------------*/

using namespace ntc;

/*------------------------------------------------------------------------------------------------*/

TEST_CASE("Encoder: create repairs")
{
  detail::galois_field gf{8};

  // Push two sources.
  detail::source_list sl;
  sl.emplace(0, detail::byte_buffer{}, 0);
  sl.emplace(1, detail::byte_buffer{}, 0);
  sl.emplace(2, detail::byte_buffer{}, 0);
  sl.emplace(3, detail::byte_buffer{}, 0);
  sl.emplace(4, detail::byte_buffer{}, 0);
  REQUIRE(sl.size() == 5);

  // A repair to store encoded sources
  detail::repair r0{0 /* id */};

  // We need an encoder to fill the repair.
  detail::encoder{8}(r0, sl.cbegin(), sl.cend());
  REQUIRE(r0.source_ids().size() == 5);
  REQUIRE(r0.source_ids()[0] == 0);
  REQUIRE(r0.source_ids()[1] == 1);
  REQUIRE(r0.source_ids()[2] == 2);
  REQUIRE(r0.source_ids()[3] == 3);
  REQUIRE(r0.source_ids()[4] == 4);
}

/*------------------------------------------------------------------------------------------------*/
