#include <algorithm> // equal

#include "tests/catch.hpp"

#include "netcode/detail/decoder.hh"
#include "netcode/detail/encoder.hh"
#include "netcode/detail/source_list.hh"

/*------------------------------------------------------------------------------------------------*/

using namespace ntc;

/*------------------------------------------------------------------------------------------------*/

TEST_CASE("Decoder: reconstruct a source from a repair")
{
  detail::galois_field gf{8};

  // The payloads that should be reconstructed.
  detail::byte_buffer s0_symbol{'a','b','c','d'};

  // Push the source.
  detail::source_list sl;
  sl.emplace(0, detail::byte_buffer{s0_symbol}, s0_symbol.size());
  REQUIRE(sl.size() == 1);

  // A repair to store encoded sources
  detail::repair r0{0 /* id */};

  // We need an encoder to fill the repair.
  detail::encoder{8}(r0, sl.cbegin(), sl.cend());
  REQUIRE(r0.source_ids().size() == 1);
  REQUIRE(r0.source_ids()[0] == 0);

  // Now test the decoder.
  detail::decoder decoder{8, [](const detail::source&){}};

  const auto s0 = decoder.create_source_from_repair(r0);
  REQUIRE(s0.user_size() == s0_symbol.size());
  REQUIRE(std::equal( s0.buffer().begin()
                      // The allocated buffer might be larger than the user size
                    , s0.buffer().begin() + s0.user_size()
                    , s0_symbol.begin()));

}

/*------------------------------------------------------------------------------------------------*/

TEST_CASE("Decoder: remove a source from a repair")
{
  detail::galois_field gf{8};

  // The payloads that should be reconstructed.
  detail::byte_buffer s0_symbol{'a','b','c','d'};
  detail::byte_buffer s1_symbol{'e','f','g','h','i'};

  // Push two sources.
  detail::source_list sl;
  sl.emplace(0, detail::byte_buffer{s0_symbol}, s0_symbol.size());
  sl.emplace(1, detail::byte_buffer{s1_symbol}, s1_symbol.size());
  REQUIRE(sl.size() == 2);

  // A repair to store encoded sources
  detail::repair r0{0 /* id */};

  // We need an encoder to fill the repair.
  detail::encoder{8}(r0, sl.cbegin(), sl.cend());
  REQUIRE(r0.source_ids().size() == 2);
  REQUIRE(r0.source_ids()[0] == 0);
  REQUIRE(r0.source_ids()[1] == 1);

  SECTION("Remove s0, we should be able to reconstruct s1")
  {
    detail::decoder decoder{8, [](const detail::source&){}};
    const detail::source s0{0, detail::byte_buffer{s0_symbol}, s0_symbol.size()};
    decoder.remove_source_from_repair(s0, r0);
    REQUIRE(r0.source_ids().size() == 1);
    REQUIRE(r0.source_ids().front() == 1);

    const auto s1 = decoder.create_source_from_repair(r0);
    REQUIRE(s1.user_size() == s1_symbol.size());
    REQUIRE(std::equal( s1.buffer().begin()
                        // The allocated buffer might be larger than the user size
                      , s1.buffer().begin() + s1.user_size()
                      , s1_symbol.begin()));
  }

  SECTION("Remove s1, we should be able to reconstruct s0")
  {
    detail::decoder decoder{8, [](const detail::source&){}};
    const detail::source s1{1, detail::byte_buffer{s1_symbol}, s1_symbol.size()};
    decoder.remove_source_from_repair(s1, r0);
    REQUIRE(r0.source_ids().size() == 1);
    REQUIRE(r0.source_ids().front() == 0);

    const auto s0 = decoder.create_source_from_repair(r0);
    REQUIRE(s0.user_size() == s0_symbol.size());
    REQUIRE(std::equal( s0.buffer().begin()
                       // The allocated buffer might be larger than the user size
                      , s0.buffer().begin() + s0.user_size()
                      , s0_symbol.begin()));
  }
}

/*------------------------------------------------------------------------------------------------*/
