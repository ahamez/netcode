#include <algorithm> // equal

#include "tests/catch.hpp"
#include "tests/netcode/launch.hh"

#include "netcode/detail/decoder.hh"
#include "netcode/detail/encoder.hh"
#include "netcode/detail/source_list.hh"

/*------------------------------------------------------------------------------------------------*/

using namespace ntc;

/*------------------------------------------------------------------------------------------------*/

TEST_CASE("Encoder: create repairs")
{
  launch([](std::uint8_t gf_size)
  {
    detail::galois_field gf{gf_size};

    // Push two sources.
    detail::source_list sl;
    sl.emplace(0, detail::byte_buffer{});
    sl.emplace(1, detail::byte_buffer{});
    sl.emplace(2, detail::byte_buffer{});
    sl.emplace(3, detail::byte_buffer{});
    sl.emplace(4, detail::byte_buffer{});
    REQUIRE(sl.size() == 5);

    // A repair to store encoded sources
    detail::repair r0{0 /* id */};

    // We need an encoder to fill the repair.
    detail::encoder{gf_size}(r0, sl);
    REQUIRE(r0.source_ids().size() == 5);
    REQUIRE(*(r0.source_ids().begin() + 0) == 0);
    REQUIRE(*(r0.source_ids().begin() + 1) == 1);
    REQUIRE(*(r0.source_ids().begin() + 2) == 2);
    REQUIRE(*(r0.source_ids().begin() + 3) == 3);
    REQUIRE(*(r0.source_ids().begin() + 4) == 4);
  });
}

/*------------------------------------------------------------------------------------------------*/

TEST_CASE("Encoder: large source")
{
  launch([](std::uint8_t gf_size)
  {
    detail::encoder enc{gf_size};

    SECTION("Largest first")
    {
      // Push sources of different sizes.
      detail::source_list sl;
      sl.emplace(0, detail::byte_buffer(8192));
      sl.emplace(1, detail::byte_buffer(128));

      // Create repair.
      detail::repair r0{0};
      enc(r0, sl);

      REQUIRE(r0.buffer().size() >= 8192);
    }

    SECTION("Smallest first")
    {
      // Push sources of different sizes.
      detail::source_list sl;
      sl.emplace(0, detail::byte_buffer(128));
      sl.emplace(1, detail::byte_buffer(8192));

      // Create repair.
      detail::repair r0{0};
      enc(r0, sl);
      
      REQUIRE(r0.buffer().size() >= 8192);
      
    }
  });
}

/*------------------------------------------------------------------------------------------------*/

TEST_CASE("Encoder is deterministic")
{
  launch([](std::uint8_t gf_size)
  {
    // We'll need two encoder to compare their evolution.
    detail::encoder encoder0{gf_size};
    detail::encoder encoder1{gf_size};

    // A dummy source.
    detail::source_list sl;
    sl.emplace(0, {'a','b','c','d'});

    // First encoder.
    detail::repair r0_0{0};
    encoder0(r0_0, sl);

    // Second encoder.
    detail::repair r0_1{0};
    encoder1(r0_1, sl);

    REQUIRE(r0_0.id() == r0_1.id());
    REQUIRE(r0_0.buffer() == r0_1.buffer());

    // Once more.
    sl.emplace(1, {'e','f','g','h'});

    // First encoder.
    detail::repair r1_0{1};
    encoder1(r1_0, sl);

    // Second encoder.
    detail::repair r1_1{1};
    encoder1(r1_1, sl);

    REQUIRE(r1_0.id() == r1_1.id());
    REQUIRE(r1_0.buffer() == r1_1.buffer());
  });
}

/*------------------------------------------------------------------------------------------------*/
