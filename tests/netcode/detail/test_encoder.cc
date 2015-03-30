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
  REQUIRE(*(r0.source_ids().begin() + 0) == 0);
  REQUIRE(*(r0.source_ids().begin() + 1) == 1);
  REQUIRE(*(r0.source_ids().begin() + 2) == 2);
  REQUIRE(*(r0.source_ids().begin() + 3) == 3);
  REQUIRE(*(r0.source_ids().begin() + 4) == 4);
}

/*------------------------------------------------------------------------------------------------*/

TEST_CASE("Encoder is deterministic")
{
  // We'll need two encoder to compare their evolution.
  detail::encoder encoder0{8};
  detail::encoder encoder1{8};

  // A dummy source.
  detail::source_list sl;
  sl.emplace(0, detail::byte_buffer{}, 0);

  // First encoder.
  detail::repair r0_0{0};
  encoder0(r0_0, sl.cbegin(), sl.cend());

  // Second encoder.
  detail::repair r0_1{0};
  encoder1(r0_1, sl.cbegin(), sl.cend());

  REQUIRE(r0_0.id() == r0_1.id());
  REQUIRE(r0_0.buffer() == r0_1.buffer());

  // Once more.
  sl.emplace(1, detail::byte_buffer{}, 0);

  // First encoder.
  detail::repair r1_0{1};
  encoder1(r1_0, sl.cbegin(), sl.cend());

  // Second encoder.
  detail::repair r1_1{1};
  encoder1(r1_1, sl.cbegin(), sl.cend());

  REQUIRE(r1_0.id() == r1_1.id());
  REQUIRE(r1_0.buffer() == r1_1.buffer());
}

/*------------------------------------------------------------------------------------------------*/
