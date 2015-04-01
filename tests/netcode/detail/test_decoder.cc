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
  // The payloads that should be reconstructed.
  detail::byte_buffer s0_data{'a','b','c','d'};

  // Push the source.
  detail::source_list sl;
  sl.emplace(0, detail::byte_buffer{s0_data}, s0_data.size());

  // A repair to store encoded sources
  detail::repair r0{0 /* id */};

  // We need an encoder to fill the repair.
  detail::encoder{8}(r0, sl);

  // Now test the decoder.
  detail::decoder decoder{8, [](const detail::source&){}};

  const auto s0 = decoder.create_source_from_repair(r0);
  REQUIRE(s0.user_size() == s0_data.size());
  REQUIRE(std::equal( s0.buffer().begin()
                      // The allocated buffer might be larger than the user size
                    , s0.buffer().begin() + s0.user_size()
                    , s0_data.begin()));

}

/*------------------------------------------------------------------------------------------------*/

TEST_CASE("Decoder: remove a source from a repair")
{
  // The payloads that should be reconstructed.
  detail::byte_buffer s0_data{'a','b','c','d'};
  detail::byte_buffer s1_data{'e','f','g','h','i'};

  // Push 2 sources.
  detail::source_list sl;
  sl.emplace(0, detail::byte_buffer{s0_data}, s0_data.size());
  sl.emplace(1, detail::byte_buffer{s1_data}, s1_data.size());

  // A repair to store encoded sources
  detail::repair r0{0 /* id */};

  // We need an encoder to fill the repair.
  detail::encoder{8}(r0, sl);

  SECTION("Remove s0, we should be able to reconstruct s1")
  {
    detail::decoder decoder{8, [](const detail::source&){}};
    const detail::source s0{0, detail::byte_buffer{s0_data}, s0_data.size()};
    decoder.remove_source_from_repair(s0, r0);
    REQUIRE(r0.source_ids().size() == 1);
    REQUIRE(*(r0.source_ids().begin()) == 1);

    const auto s1 = decoder.create_source_from_repair(r0);
    REQUIRE(s1.user_size() == s1_data.size());
    REQUIRE(std::equal( s1.buffer().begin()
                        // The allocated buffer might be larger than the user size
                      , s1.buffer().begin() + s1.user_size()
                      , s1_data.begin()));
  }

  SECTION("Remove s1, we should be able to reconstruct s0")
  {
    detail::decoder decoder{8, [](const detail::source&){}};
    const detail::source s1{1, detail::byte_buffer{s1_data}, s1_data.size()};
    decoder.remove_source_from_repair(s1, r0);
    REQUIRE(r0.source_ids().size() == 1);
    REQUIRE(*(r0.source_ids().begin()) == 0);

    const auto s0 = decoder.create_source_from_repair(r0);
    REQUIRE(s0.user_size() == s0_data.size());
    REQUIRE(std::equal( s0.buffer().begin()
                       // The allocated buffer might be larger than the user size
                      , s0.buffer().begin() + s0.user_size()
                      , s0_data.begin()));
  }
}

/*------------------------------------------------------------------------------------------------*/

TEST_CASE("Decoder: useless repair")
{
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
  detail::encoder{8}(r0, sl);

  // Now test the decoder.
  detail::decoder decoder{8, [](const detail::source&){}};
  decoder(detail::source{0, detail::byte_buffer{}, 0});
  decoder(detail::source{1, detail::byte_buffer{}, 0});
  decoder(detail::source{2, detail::byte_buffer{}, 0});
  decoder(detail::source{3, detail::byte_buffer{}, 0});
  decoder(detail::source{4, detail::byte_buffer{}, 0});
  decoder(std::move(r0));
  REQUIRE(decoder.sources().size() == 5);
  REQUIRE(decoder.missing_sources().empty());
  REQUIRE(decoder.repairs().size() == 0);
  REQUIRE(decoder.nb_useless_repairs() == 1);
}

/*------------------------------------------------------------------------------------------------*/

TEST_CASE("Decoder: missing sources")
{
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
  detail::encoder{8}(r0, sl);

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

TEST_CASE("Decoder: drop outdated sources")
{
  // We need an encoder to fill repairs.
  detail::encoder encoder{8};

  // The decoder to test
  detail::decoder decoder{8, [](const detail::source&){}};

  // Send some sources to the decoder.
  decoder(detail::source{0, detail::byte_buffer{}, 0});
  decoder(detail::source{1, detail::byte_buffer{}, 0});
  REQUIRE(decoder.sources().size() == 2);

  // Now create a repair that acknowledges the 2 first sources.
  detail::source_list sl;
  sl.emplace(2, detail::byte_buffer{}, 0);
  sl.emplace(3, detail::byte_buffer{}, 0);
  sl.emplace(4, detail::byte_buffer{}, 0);
  detail::repair r0{0};
  encoder(r0, sl);

  SECTION("sources lost")
  {
    // Send repair.
    decoder(std::move(r0));

    // Now test the decoder.
    REQUIRE(decoder.sources().size() == 0);
    REQUIRE(decoder.missing_sources().size() == 3);
    REQUIRE(decoder.missing_sources().count(2));
    REQUIRE(decoder.missing_sources().count(3));
    REQUIRE(decoder.missing_sources().count(4));
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
    REQUIRE(decoder.sources().count(2));
    REQUIRE(decoder.sources().count(3));
    REQUIRE(decoder.sources().count(4));
    REQUIRE(decoder.missing_sources().size() == 0);
    REQUIRE(decoder.repairs().size() == 0);
    REQUIRE(decoder.nb_useless_repairs() == 1);
  }
}

/*------------------------------------------------------------------------------------------------*/

TEST_CASE("Decoder: drop outdated lost sources")
{
  // We need an encoder to fill repairs.
  detail::encoder encoder{8};

  // The decoder to test
  detail::decoder decoder{8, [](const detail::source&){}};

  // A repair with the first 2 sources.
  detail::source_list sl0;
  sl0.emplace(0, detail::byte_buffer{}, 0);
  sl0.emplace(1, detail::byte_buffer{}, 0);
  detail::repair r0{0};
  encoder(r0, sl0);

  // First 2 sources are lost.
  decoder(std::move(r0));
  REQUIRE(decoder.missing_sources().size() == 2);
  REQUIRE(decoder.missing_sources().count(0));
  REQUIRE(decoder.missing_sources().count(1));
  REQUIRE(decoder.repairs().count(0));
  REQUIRE(decoder.repairs().find(0)->second.source_ids().size() > 0);

  // Now create a repair that drops the 2 first sources due to a limited window size.
  detail::source_list sl1;
  sl1.emplace(2, detail::byte_buffer{}, 0);
  sl1.emplace(3, detail::byte_buffer{}, 0);
  detail::repair r1{1};
  encoder(r1, sl1);

  SECTION("sources lost")
  {
    // Send repair.
    decoder(std::move(r1));

    // Now test the decoder.
    REQUIRE(decoder.sources().empty());
    // s2 and s3 are missing.
    REQUIRE(decoder.missing_sources().size() == 2);
    REQUIRE(decoder.missing_sources().count(2));
    REQUIRE(decoder.missing_sources().count(3));
    // r0 should have been dropped.
    REQUIRE(decoder.repairs().size() == 1);
    REQUIRE(decoder.nb_useless_repairs() == 0);
  }

  SECTION("sources received")
  {
    // Send sources
    decoder(detail::source{2, detail::byte_buffer{}, 0});
    decoder(detail::source{3, detail::byte_buffer{}, 0});

    // Send repair.
    decoder(std::move(r1));

    // Now test the decoder.
    REQUIRE(decoder.sources().size() == 2);
    REQUIRE(decoder.sources().count(2));
    REQUIRE(decoder.sources().count(3));
    REQUIRE(decoder.missing_sources().empty());
    REQUIRE(decoder.nb_useless_repairs() == 1);
    // r0 should have been dropped and r1 is useless
    REQUIRE(decoder.repairs().size() == 0);
  }
}

/*------------------------------------------------------------------------------------------------*/

TEST_CASE("Decoder: one source lost encoded in one received repair")
{
  // The payloads that should be reconstructed.
  detail::byte_buffer s0_data{'a','b','c','d'};

  // Push the source.
  detail::source_list sl;
  sl.emplace(0, detail::byte_buffer{s0_data}, s0_data.size());

  // A repair to store encoded sources
  detail::repair r0{0 /* id */};

  // We need an encoder to fill the repair.
  detail::encoder{8}(r0, sl);

  // Now test the decoder.
  detail::decoder decoder{8, [&](const detail::source& s0)
                                {
                                  REQUIRE(s0.id() == 0);
                                  REQUIRE(s0.user_size() == s0_data.size());
                                  REQUIRE(std::equal( s0.buffer().begin()
                                                    , s0.buffer().begin() + s0.user_size()
                                                    , s0_data.begin()));
                                }};
  decoder(std::move(r0));
}

/*------------------------------------------------------------------------------------------------*/

TEST_CASE("Decoder: 2 lost sources from 2 repairs")
{
  detail::encoder encoder{8};
  detail::decoder decoder{8, [&](const detail::source&){}};

  // The payloads that should be reconstructed.
  detail::byte_buffer s0_data{'a','b','c','d'};
  detail::byte_buffer s1_data{'e','f','g','h','i'};

  // Push 2 sources.
  detail::source_list sl;
  sl.emplace(0, detail::byte_buffer{s0_data}, s0_data.size());
  sl.emplace(1, detail::byte_buffer{s1_data}, s1_data.size());

  // 2 repairs to store encoded sources
  detail::repair r0{0 /* id */};
  detail::repair r1{1 /* id */};
  encoder(r0, sl);
  encoder(r1, sl);

  // Send first repair.
  decoder(std::move(r0));
  REQUIRE(decoder.sources().empty());
  REQUIRE(decoder.missing_sources().size() == 2);
  REQUIRE(decoder.missing_sources().count(0));
  REQUIRE(decoder.missing_sources().count(1));
  REQUIRE(decoder.repairs().size() == 1);
  REQUIRE(decoder.repairs().count(0));

  // Send second repair, full decoding should take place.
  decoder(std::move(r1));
  REQUIRE(decoder.sources().size() == 2);
  REQUIRE(decoder.sources().count(0));
  REQUIRE(decoder.sources().count(1));
  REQUIRE(decoder.missing_sources().size() == 0);
  REQUIRE(decoder.repairs().size() == 0);

  // Now, check contents.
  REQUIRE(decoder.sources().find(0)->second.user_size() == s0_data.size());
  REQUIRE( std::equal( s0_data.begin(), s0_data.end()
                     , decoder.sources().find(0)->second.buffer().begin()));
  REQUIRE(decoder.sources().find(1)->second.user_size() == s1_data.size());
  REQUIRE( std::equal( s1_data.begin(), s1_data.end()
                     , decoder.sources().find(1)->second.buffer().begin()));
}

/*------------------------------------------------------------------------------------------------*/

// Tests might broke if coefficient generator is changed as the coefficient matrix might not be
// invertible.
TEST_CASE("Decoder: several lost sources from several repairs")
{
  detail::encoder encoder{8};
  detail::decoder decoder{8, [&](const detail::source&){}};

  // The payloads that should be reconstructed.
  detail::byte_buffer s0_data{'a','b','c','d'};
  detail::byte_buffer s1_data{'e','f','g','h','i'};
  detail::byte_buffer s2_data{'j','k','l','m','n','o','p'};
  detail::byte_buffer s3_data{'q','r','s'};
  detail::byte_buffer s4_data{'t','u','v','w','x','y','z'};

  // Push 2 sources.
  detail::source_list sl;
  sl.emplace(0, detail::byte_buffer{s0_data}, s0_data.size());
  sl.emplace(1, detail::byte_buffer{s1_data}, s1_data.size());

  // 2 repairs to store encoded sources
  detail::repair r0{0};
  detail::repair r1{1};
  encoder(r0, sl);
  encoder(r1, sl);

  // Send first repair.
  decoder(std::move(r0));
  REQUIRE(decoder.sources().empty());
  REQUIRE(decoder.missing_sources().size() == 2);
  REQUIRE(decoder.missing_sources().count(0));
  REQUIRE(decoder.missing_sources().count(1));
  REQUIRE(decoder.repairs().size() == 1);
  REQUIRE(decoder.repairs().count(0));

  // Send second repair, full decoding should take place.
  decoder(std::move(r1));
  REQUIRE(decoder.sources().size() == 2);
  REQUIRE(decoder.sources().count(0));
  REQUIRE(decoder.sources().count(1));
  REQUIRE(decoder.missing_sources().size() == 0);
  REQUIRE(decoder.repairs().size() == 0);

  // Now, check contents.
  REQUIRE(decoder.sources().find(0)->second.user_size() == s0_data.size());
  REQUIRE(std::equal( s0_data.begin(), s0_data.end()
                    , decoder.sources().find(0)->second.buffer().begin()));
  REQUIRE(decoder.sources().find(1)->second.user_size() == s1_data.size());
  REQUIRE(std::equal( s1_data.begin(), s1_data.end()
                    , decoder.sources().find(1)->second.buffer().begin()));

  SECTION("Sources are not outdated")
  {
    auto nb_failed_full_decodings = 0ul;

    // More repairs.
    detail::repair r2{2};
    detail::repair r3{3};
    detail::repair r4{4};

    // Push 3 new sources.
    sl.emplace(2, detail::byte_buffer{s2_data}, s2_data.size());
    sl.emplace(3, detail::byte_buffer{s3_data}, s3_data.size());
    encoder(r2, sl);
    sl.emplace(4, detail::byte_buffer{s4_data}, s4_data.size());
    encoder(r3, sl);
    encoder(r4, sl);

    // Send 1 more repair, there should not be any decoding.
    decoder(std::move(r2));
    REQUIRE(decoder.sources().size() == 2);
    REQUIRE(decoder.sources().count(0));
    REQUIRE(decoder.sources().count(1));
    REQUIRE(decoder.missing_sources().size() == 2);
    REQUIRE(decoder.repairs().size() == 1);

    // Send 1 more repair, there should not be any decoding.
    decoder(std::move(r3));
    REQUIRE(decoder.sources().size() == 2);
    REQUIRE(decoder.sources().count(0));
    REQUIRE(decoder.sources().count(1));
    REQUIRE(decoder.missing_sources().size() == 3);
    REQUIRE(decoder.repairs().size() == 2);

    // Send 1 more repair, full decoding could take place.
    decoder(std::move(r4));
    if (decoder.nb_failed_full_decodings() != nb_failed_full_decodings)
    {
      // Previous decoding attempt failed.
      REQUIRE(decoder.repairs().size() == 2);

      ++nb_failed_full_decodings;
      // Try with a new repair.
      detail::repair r5{5};
      encoder(r5, sl);
      decoder(std::move(r5));
      if (decoder.nb_failed_full_decodings() != nb_failed_full_decodings)
      {
        REQUIRE(false); // Failure to decode, again ?!!
      }

      REQUIRE(decoder.sources().size() == 5);
      REQUIRE(decoder.sources().count(0));
      REQUIRE(decoder.sources().count(1));
      REQUIRE(decoder.sources().count(2));
      REQUIRE(decoder.sources().count(3));
      REQUIRE(decoder.sources().count(4));
      REQUIRE(decoder.missing_sources().size() == 0);
      REQUIRE(decoder.repairs().size() == 0);


      // Check contents.
      REQUIRE(decoder.sources().find(2)->second.user_size() == s2_data.size());
      REQUIRE(std::equal( s2_data.begin(), s2_data.end()
                        , decoder.sources().find(2)->second.buffer().begin()));
      REQUIRE(decoder.sources().find(3)->second.user_size() == s3_data.size());
      REQUIRE(std::equal( s3_data.begin(), s3_data.end()
                        , decoder.sources().find(3)->second.buffer().begin()));
      REQUIRE(decoder.sources().find(4)->second.user_size() == s4_data.size());
      REQUIRE(std::equal( s4_data.begin(), s4_data.end()
                        , decoder.sources().find(4)->second.buffer().begin()));

    }
  }

  SECTION("Sources are outdated")
  {
    auto nb_failed_full_decodings = 0ul;

    // More repairs.
    detail::repair r2{2};
    detail::repair r3{3};
    detail::repair r4{4};

    // Ack : remove the 2 previously sent sources, thus they won't be encoded in following repairs.
    sl.pop_front();
    sl.pop_front();

    // Push 3 new sources.
    sl.emplace(2, detail::byte_buffer{s2_data}, s2_data.size());
    sl.emplace(3, detail::byte_buffer{s3_data}, s3_data.size());
    encoder(r2, sl);
    sl.emplace(4, detail::byte_buffer{s4_data}, s4_data.size());
    encoder(r3, sl);
    encoder(r4, sl);

    // Send 1 more repair, there should not be any decoding.
    decoder(std::move(r2));
    REQUIRE(decoder.sources().size() == 0);
    REQUIRE(decoder.missing_sources().size() == 2);
    REQUIRE(decoder.repairs().size() == 1);

    // Send 1 more repair, there should not be any decoding.
    decoder(std::move(r3));
    REQUIRE(decoder.sources().size() == 0);
    REQUIRE(decoder.missing_sources().size() == 3);
    REQUIRE(decoder.repairs().size() == 2);

    // Send 1 more repair, full decoding could take place.
    decoder(std::move(r4));
    if (decoder.nb_failed_full_decodings() != nb_failed_full_decodings)
    {
      // Previous decoding attempt failed.
      REQUIRE(decoder.repairs().size() == 2);

      ++nb_failed_full_decodings;
      // Try with a new repair.
      detail::repair r5{5};
      encoder(r5, sl);
      decoder(std::move(r5));
      if (decoder.nb_failed_full_decodings() != nb_failed_full_decodings)
      {
        REQUIRE(false); // Failure to decode, again ?!!
      }
    }

    REQUIRE(decoder.sources().size() == 3);
    REQUIRE(decoder.sources().count(2));
    REQUIRE(decoder.sources().count(3));
    REQUIRE(decoder.sources().count(4));
    REQUIRE(decoder.missing_sources().size() == 0);
    REQUIRE(decoder.repairs().size() == 0);

    // Check contents.
    REQUIRE(decoder.sources().find(2)->second.user_size() == s2_data.size());
    REQUIRE(std::equal( s2_data.begin(), s2_data.end()
                      , decoder.sources().find(2)->second.buffer().begin()));
    REQUIRE(decoder.sources().find(3)->second.user_size() == s3_data.size());
    REQUIRE(std::equal( s3_data.begin(), s3_data.end()
                      , decoder.sources().find(3)->second.buffer().begin()));
    REQUIRE(decoder.sources().find(4)->second.user_size() == s4_data.size());
    REQUIRE(std::equal( s4_data.begin(), s4_data.end()
                      , decoder.sources().find(4)->second.buffer().begin()));
  }
}

/*------------------------------------------------------------------------------------------------*/

TEST_CASE("Decoder: duplicate source")
{
  detail::decoder decoder{8, [](const detail::source&){}};

  // Send source.
  decoder(detail::source{0, detail::byte_buffer{}, 0});
  REQUIRE(decoder.sources().size() == 1);
  REQUIRE(decoder.missing_sources().size() == 0);
  REQUIRE(decoder.repairs().size() == 0);
  REQUIRE(decoder.nb_useless_repairs() == 0);

  // Send duplicate source.
  decoder(detail::source{0, detail::byte_buffer{}, 0});
  REQUIRE(decoder.sources().size() == 1);
  REQUIRE(decoder.missing_sources().size() == 0);
  REQUIRE(decoder.repairs().size() == 0);
  REQUIRE(decoder.nb_useless_repairs() == 0);
}

/*------------------------------------------------------------------------------------------------*/

TEST_CASE("Decoder: out-of-order source after repair")
{
  detail::encoder encoder{8};
  detail::decoder decoder{8, [](const detail::source&){}};

  detail::source_list sl;
  sl.emplace(0, detail::byte_buffer{}, 0);
  detail::repair r0{0};
  encoder(r0, sl);

  // Send repair
  decoder(std::move(r0));
  REQUIRE(decoder.sources().size() == 1);
  REQUIRE(decoder.sources().count(0));

  SECTION("Lost source is not outdated")
  {
    // Eventually, the missing source is received.
    decoder(detail::source{0, detail::byte_buffer{}, 0});
    REQUIRE(decoder.sources().size() == 1);
    REQUIRE(decoder.sources().count(0));
  }

  SECTION("Lost source is outdated")
  {
    // No more s0 on encoder side.
    sl.pop_front();

    // A new source along with a new repair.
    sl.emplace(1, detail::byte_buffer{}, 0);
    detail::repair r1{0};
    encoder(r1, sl);

    // Send repair
    decoder(std::move(r1));
    REQUIRE(decoder.sources().size() == 1);
    REQUIRE(decoder.sources().count(1));

    // Eventually, the missing source is received.
    decoder(detail::source{0, detail::byte_buffer{}, 0});
    REQUIRE(decoder.sources().size() == 1);
    REQUIRE(decoder.sources().count(1));
  }
}

/*------------------------------------------------------------------------------------------------*/

TEST_CASE("Decoder: duplicate repair 1")
{
  // We'll need two encoders as we can't copy a repair.
  detail::encoder encoder0{8};
  detail::encoder encoder1{8};

  detail::decoder decoder{8, [](const detail::source&){}};

  // A dummy lost source. Should be repaired immediatly.
  detail::source_list sl;
  sl.emplace(0, detail::byte_buffer{}, 0);

  // Create original repair.
  detail::repair r0{0};
  encoder0(r0, sl);

  // Create copy.
  detail::repair r0_dup{0};
  encoder1(r0_dup, sl);

  // Send original repair.
  decoder(std::move(r0));
  REQUIRE(decoder.sources().size() == 1);
  REQUIRE(decoder.missing_sources().size() == 0);
  REQUIRE(decoder.repairs().size() == 0);
  REQUIRE(decoder.nb_useless_repairs() == 0);

  SECTION("Reconstructed source is not outdated")
  {
    // Now send duplicate. Should be seen as useless.
    decoder(std::move(r0_dup));
    REQUIRE(decoder.sources().size() == 1);
    REQUIRE(decoder.missing_sources().size() == 0);
    REQUIRE(decoder.repairs().size() == 0);
    REQUIRE(decoder.nb_useless_repairs() == 1);
  }

  SECTION("Reconstructed source is outdated")
  {
    sl.pop_front();
    sl.emplace(1, detail::byte_buffer{}, 0);
    detail::repair r1{0};
    encoder0(r1, sl);
    // Send repair.
    decoder(std::move(r1));

    // Now send duplicate.
    decoder(std::move(r0_dup));
    REQUIRE(decoder.sources().size() == 1);
    REQUIRE(decoder.sources().count(1));
    REQUIRE(decoder.missing_sources().size() == 0);
    REQUIRE(decoder.repairs().size() == 0);
    REQUIRE(decoder.nb_useless_repairs() == 0);
  }
}

/*------------------------------------------------------------------------------------------------*/

TEST_CASE("Decoder: duplicate repair 2")
{
  // We'll need two encoders as we can't copy a repair.
  detail::encoder encoder0{8};
  detail::encoder encoder1{8};

  detail::decoder decoder{8, [](const detail::source&){}};

  // Some dummy lost sources.
  detail::source_list sl;
  sl.emplace(0, detail::byte_buffer{}, 0);
  sl.emplace(1, detail::byte_buffer{}, 0);

  // Create original repair.
  detail::repair r0{0};
  encoder0(r0, sl);

  // Create copy.
  detail::repair r0_dup{0};
  encoder1(r0_dup, sl);

  // Send original repair.
  decoder(std::move(r0));
  REQUIRE(decoder.sources().size() == 0);
  REQUIRE(decoder.missing_sources().size() == 2);
  REQUIRE(decoder.repairs().size() == 1);
  REQUIRE(decoder.nb_useless_repairs() == 0);

  // Send duplicate.
  decoder(std::move(r0_dup));
  REQUIRE(decoder.sources().size() == 0);
  REQUIRE(decoder.missing_sources().size() == 2);
  REQUIRE(decoder.repairs().size() == 1);
  REQUIRE(decoder.nb_useless_repairs() == 0);
}

/*------------------------------------------------------------------------------------------------*/

TEST_CASE("Decoder: source after repair")
{
  detail::encoder encoder{8};
  detail::decoder decoder{8, [&](const detail::source&){}};

  // The payloads that should be reconstructed.
  detail::byte_buffer s0_data{'a','b','c','d'};
  detail::byte_buffer s1_data{'e','f','g','h','i'};

  // Push 2 sources.
  detail::source_list sl;
  sl.emplace(0, detail::byte_buffer{s0_data}, s0_data.size());
  sl.emplace(1, detail::byte_buffer{s1_data}, s1_data.size());

  // 2 repairs to store encoded sources
  detail::repair r0{0};
  detail::repair r1{1};
  encoder(r0, sl);
  encoder(r1, sl);

  // r0 is received before s0 and s1
  decoder(std::move(r0));
  REQUIRE(decoder.sources().size() == 0);
  REQUIRE(decoder.missing_sources().size() == 2);
  REQUIRE(decoder.repairs().size() == 1);

  // s0 is received
  decoder({0, detail::byte_buffer{s0_data}, s0_data.size()});
  REQUIRE(decoder.sources().size() == 2);
  REQUIRE(decoder.sources().count(0));
  REQUIRE(decoder.sources().count(1));
  REQUIRE(decoder.missing_sources().size() == 0);
  REQUIRE(decoder.repairs().size() == 0);
}

/*------------------------------------------------------------------------------------------------*/

TEST_CASE("Decoder: repair with only one source")
{
  // The payloads that should be reconstructed.
  detail::byte_buffer s0_data{'a','b','c','d'};

  // Push the source.
  detail::source_list sl;
  sl.emplace(0, detail::byte_buffer{s0_data}, s0_data.size());

  // A repair to store encoded sources
  detail::repair r0{0};

  // We need an encoder to fill the repair.
  detail::encoder{8}(r0, sl);

  // Now test the decoder.
  detail::decoder decoder{8, [](const detail::source&){}};

  // r0 is received
  decoder(std::move(r0));
  REQUIRE(decoder.sources().size() == 1);
  REQUIRE(decoder.sources().count(0));
  REQUIRE(decoder.sources().find(0)->second.user_size() == s0_data.size());
  REQUIRE(std::equal( s0_data.begin(), s0_data.end()
                    , decoder.sources().find(0)->second.buffer().begin()));
  REQUIRE(decoder.missing_sources().size() == 0);
  REQUIRE(decoder.repairs().size() == 0);
}

/*------------------------------------------------------------------------------------------------*/

TEST_CASE("Decoder: 1 packet loss")
{
  // The payloads that should be reconstructed.
  detail::byte_buffer s0_data{'a','b','c',};
  detail::byte_buffer s1_data{'d','e','f'};
  detail::byte_buffer s2_data{'g','h','i'};
  detail::byte_buffer s3_data{'j','k','l'};

  // Push the source.
  detail::source_list sl;
  sl.emplace(0, detail::byte_buffer{s0_data}, s0_data.size());
  sl.emplace(1, detail::byte_buffer{s0_data}, s0_data.size());
  sl.emplace(2, detail::byte_buffer{s0_data}, s0_data.size());
  sl.emplace(3, detail::byte_buffer{s0_data}, s0_data.size());

  // A repair to store encoded sources
  detail::repair r0{0};

  // We need an encoder to fill the repair.
  detail::encoder{8}(r0, sl);

  // Now test the decoder.
  detail::decoder decoder{8, [](const detail::source&){}};

  // s1 -> s3 are received
  decoder(detail::source{1, detail::byte_buffer{s1_data}, s1_data.size()});
  decoder(detail::source{2, detail::byte_buffer{s2_data}, s2_data.size()});
  decoder(detail::source{3, detail::byte_buffer{s3_data}, s3_data.size()});
  REQUIRE(decoder.sources().size() == 3);
  REQUIRE(decoder.sources().count(1));
  REQUIRE(decoder.sources().count(2));
  REQUIRE(decoder.sources().count(3));


  // r0 is received
  decoder(std::move(r0));
  REQUIRE(decoder.sources().size() == 4);
  REQUIRE(decoder.sources().count(0));
  REQUIRE(decoder.sources().count(1));
  REQUIRE(decoder.sources().count(2));
  REQUIRE(decoder.sources().count(3));
  REQUIRE(decoder.repairs().size() == 0);
  REQUIRE(decoder.nb_useless_repairs() == 0);
  REQUIRE(decoder.nb_failed_full_decodings() == 0);

}

/*------------------------------------------------------------------------------------------------*/
