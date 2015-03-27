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

  // A repair to store encoded sources
  detail::repair r0{0 /* id */};

  // We need an encoder to fill the repair.
  detail::encoder{8}(r0, sl.cbegin(), sl.cend());

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

  // Push 2 sources.
  detail::source_list sl;
  sl.emplace(0, detail::byte_buffer{s0_symbol}, s0_symbol.size());
  sl.emplace(1, detail::byte_buffer{s1_symbol}, s1_symbol.size());

  // A repair to store encoded sources
  detail::repair r0{0 /* id */};

  // We need an encoder to fill the repair.
  detail::encoder{8}(r0, sl.cbegin(), sl.cend());

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

TEST_CASE("Decoder: useless repair")
{
  detail::galois_field gf{8};

  // Push 5 sources.
  detail::source_list sl;
  sl.emplace(0, detail::byte_buffer{}, 0);
  sl.emplace(1, detail::byte_buffer{}, 0);
  sl.emplace(2, detail::byte_buffer{}, 0);
  sl.emplace(3, detail::byte_buffer{}, 0);
  sl.emplace(4, detail::byte_buffer{}, 0);

  // A repair to store encoded sources
  detail::repair r0{0 /* id */};

  // We need an encoder to fill the repair.
  detail::encoder{8}(r0, sl.cbegin(), sl.cend());

  // Now test the decoder.
  detail::decoder decoder{8, [](const detail::source&){}};
  decoder(detail::source{0, detail::byte_buffer{}, 0});
  decoder(detail::source{1, detail::byte_buffer{}, 0});
  decoder(detail::source{2, detail::byte_buffer{}, 0});
  decoder(detail::source{3, detail::byte_buffer{}, 0});
  decoder(detail::source{4, detail::byte_buffer{}, 0});
  decoder(std::move(r0));
  REQUIRE(decoder.sources().size() == 5);
  REQUIRE(decoder.missing_sources().size() == 0);
  REQUIRE(decoder.repairs().size() == 0);
  REQUIRE(decoder.nb_useless_repairs() == 1);
}

/*------------------------------------------------------------------------------------------------*/

TEST_CASE("Decoder: missing sources")
{
  detail::galois_field gf{8};

  // Push 5 sources.
  detail::source_list sl;
  sl.emplace(0, detail::byte_buffer{}, 0);
  sl.emplace(1, detail::byte_buffer{}, 0);
  sl.emplace(2, detail::byte_buffer{}, 0);
  sl.emplace(3, detail::byte_buffer{}, 0);
  sl.emplace(4, detail::byte_buffer{}, 0);

  // A repair to store encoded sources
  detail::repair r0{0 /* id */};

  // We need an encoder to fill the repair.
  detail::encoder{8}(r0, sl.cbegin(), sl.cend());

  // Now test the decoder.
  detail::decoder decoder{8, [](const detail::source&){}};
  decoder(detail::source{0, detail::byte_buffer{}, 0});
  decoder(detail::source{2, detail::byte_buffer{}, 0});
  decoder(detail::source{4, detail::byte_buffer{}, 0});
  decoder(std::move(r0));
  REQUIRE(decoder.sources().size() == 3);
  REQUIRE(decoder.missing_sources().size() == 2);
  REQUIRE(decoder.repairs().size() == 1);
  REQUIRE(decoder.nb_useless_repairs() == 0);
}

/*------------------------------------------------------------------------------------------------*/

TEST_CASE("Decoder: drop old sources")
{
  detail::galois_field gf{8};

  // We need an encoder to fill repairs.
  detail::encoder encoder{8};

  // The decoder to test
  detail::decoder decoder{8, [](const detail::source&){}};

  // Send some sources to the decoder.
  decoder(detail::source{0, detail::byte_buffer{}, 0});
  decoder(detail::source{1, detail::byte_buffer{}, 0});

  // Now create a repair that acknowledges the 2 first sources.
  detail::source_list sl;
  sl.emplace(2, detail::byte_buffer{}, 0);
  sl.emplace(3, detail::byte_buffer{}, 0);
  sl.emplace(4, detail::byte_buffer{}, 0);
  detail::repair r0{0};
  encoder(r0, sl.cbegin(), sl.cend());

  SECTION("sources lost")
  {
    // Send repair.
    decoder(std::move(r0));

    // Now test the decoder.
    REQUIRE(decoder.sources().size() == 0);
    REQUIRE(decoder.missing_sources().size() == 3);
    REQUIRE(decoder.missing_sources().find(2) != decoder.missing_sources().end());
    REQUIRE(decoder.missing_sources().find(3) != decoder.missing_sources().end());
    REQUIRE(decoder.missing_sources().find(4) != decoder.missing_sources().end());
    REQUIRE(decoder.repairs().size() == 1);
    REQUIRE(decoder.nb_useless_repairs() == 0);
  }

  SECTION("sources received")
  {
    // Send sources
    decoder(detail::source{2, detail::byte_buffer{}, 0});
    decoder(detail::source{3, detail::byte_buffer{}, 0});
    decoder(detail::source{4, detail::byte_buffer{}, 0});

    // Send repair.
    decoder(std::move(r0));

    // Now test the decoder.
    REQUIRE(decoder.sources().size() == 3);
    REQUIRE(decoder.sources().find(2) != decoder.sources().end());
    REQUIRE(decoder.sources().find(3) != decoder.sources().end());
    REQUIRE(decoder.sources().find(4) != decoder.sources().end());
    REQUIRE(decoder.missing_sources().size() == 0);
    REQUIRE(decoder.repairs().size() == 0);
    REQUIRE(decoder.nb_useless_repairs() == 1);
  }
}

/*------------------------------------------------------------------------------------------------*/

TEST_CASE("Decoder: one source lost encoded in one received repair")
{
  detail::galois_field gf{8};

  // The payloads that should be reconstructed.
  detail::byte_buffer s0_symbol{'a','b','c','d'};

  // Push the source.
  detail::source_list sl;
  sl.emplace(0, detail::byte_buffer{s0_symbol}, s0_symbol.size());

  // A repair to store encoded sources
  detail::repair r0{0 /* id */};

  // We need an encoder to fill the repair.
  detail::encoder{8}(r0, sl.cbegin(), sl.cend());

  // Now test the decoder.
  detail::decoder decoder{8, [&](const detail::source& s0)
                                {
                                  REQUIRE(s0.id() == 0);
                                  REQUIRE(s0.user_size() == s0_symbol.size());
                                  REQUIRE(std::equal( s0.buffer().begin()
                                                    , s0.buffer().begin() + s0.user_size()
                                                    , s0_symbol.begin()));
                                }};
  decoder(std::move(r0));
}

/*------------------------------------------------------------------------------------------------*/
