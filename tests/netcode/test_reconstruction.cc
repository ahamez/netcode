#include <iostream>

#include "tests/catch.hpp"

#include "netcode/detail/encoder.hh"
#include "netcode/detail/galois_field.hh"
#include "netcode/detail/invert_matrix.hh"
#include "netcode/detail/repair.hh"
#include "netcode/detail/source.hh"
#include "netcode/detail/source_list.hh"
#include "netcode/detail/square_matrix.hh"

/*------------------------------------------------------------------------------------------------*/

using namespace ntc;

/*------------------------------------------------------------------------------------------------*/

TEST_CASE("Encode one source")
{
  detail::galois_field gf{8};

  // The payload that should be reconstructed.
  detail::byte_buffer s0_symbol{'a','b','c','d'};

  // Push some sources.
  detail::source_list sl;
  sl.emplace(0, detail::byte_buffer{s0_symbol}, 4);
  REQUIRE(sl.size() == 1);

  // A repair to store encoded sources
  detail::repair r0{0};

  // We need an encoder to fill the repair.
  detail::encoder{8}(r0, sl.cbegin(), sl.cend());
  REQUIRE(r0.source_ids()[0] == 0);

  // The inverse of the coefficient.
  const auto inv = gf.divide(1, detail::coefficient(gf, 0, 0));

  // s0 is lost, reconstruct it in a new source.

  // First, compute its size.
  const auto src_size = gf.multiply(inv, static_cast<std::uint32_t>(r0.size()));
  REQUIRE(src_size == 4);

  detail::source s0{0, detail::byte_buffer{'x','x','x','x'}, src_size};
  gf.multiply(r0.buffer().data(), s0.buffer().data(), src_size, inv);
  REQUIRE(s0.buffer() == s0_symbol);
  REQUIRE(s0.buffer().size() == src_size);
}

/*------------------------------------------------------------------------------------------------*/

TEST_CASE("Encode two sizes")
{
  detail::galois_field gf{8};

  const std::uint32_t c0 = detail::coefficient(gf, 0 /*repair*/, 0 /*src*/);
  const std::uint32_t s0 = 4;

  // Initialize with size of s0.
  auto r0 = gf.multiply(c0, s0);

  const auto inv0 = gf.divide(1, c0);
  REQUIRE(gf.multiply(inv0, r0) == 4);


  const std::uint32_t c1 = detail::coefficient(gf, 0 /*repair*/, 1 /*src*/);
  const std::uint32_t s1 = 5;

  // Add size of s1.
  r0 ^= gf.multiply(c1, s1);

  const auto inv1 = gf.divide(1, c1);

  SECTION("Remove s0 (s1 is lost)")
  {
    // Remove s0.
    r0 ^= gf.multiply(c0, s0);

    REQUIRE(gf.multiply(inv1, r0) == 5);
  }

  SECTION("Remove s1 (s0 is lost)")
  {
    // Remove s1.
    r0 ^= gf.multiply(c1, s1);

    REQUIRE(gf.multiply(inv0, r0) == 4);
  }
}

/*------------------------------------------------------------------------------------------------*/

TEST_CASE("Encode two sources")
{
  detail::galois_field gf{8};

  // The payloads that should be reconstructed.
  detail::byte_buffer s0_symbol{'a','b','c','d'};
  detail::byte_buffer s1_symbol{'e','f','g','h','i'};

  // The coefficients.
  const auto c0 = detail::coefficient(gf, 0 /*repair*/, 0 /*src*/);
  const auto c1 = detail::coefficient(gf, 0 /*repair*/, 1 /*src*/);

  // Push two sources.
  detail::source_list sl;
  sl.emplace(0, detail::byte_buffer{s0_symbol}, 4);
  sl.emplace(1, detail::byte_buffer{s1_symbol}, 5);
  REQUIRE(sl.size() == 2);

  // A repair to store encoded sources
  detail::repair r0{0};

  // We need an encoder to fill the repair.
  detail::encoder{8}(r0, sl.cbegin(), sl.cend());
  REQUIRE(r0.source_ids()[0] == 0);
  REQUIRE(r0.source_ids()[1] == 1);

  SECTION("s0 is lost")
  {
    // First, remove size.
    r0.size() ^= gf.multiply(c1, static_cast<std::uint32_t>(s1_symbol.size()));

    // Second, remove symbol.
    gf.multiply_add(s1_symbol.data(), r0.buffer().data(), s1_symbol.size(), c1);

    // The inverse of the coefficient.
    const auto inv0 = gf.divide(1, c0);

    // First, compute its size.
    const auto src_size = gf.multiply(inv0, static_cast<std::uint32_t>(r0.size()));
    REQUIRE(src_size == s0_symbol.size());

    // Now, reconstruct missing symbol.
    detail::source s0{0, detail::byte_buffer{'x','x','x','x'}, src_size};
    gf.multiply(r0.buffer().data(), s0.buffer().data(), s0_symbol.size(), inv0);
    REQUIRE(s0.buffer() == s0_symbol);
  }

  SECTION("s1 is lost")
  {
    // First, remove size.
    r0.size() ^= gf.multiply(c0, static_cast<std::uint32_t>(s0_symbol.size()));
    // Second, remove symbol.
    gf.multiply_add(s0_symbol.data(), r0.buffer().data(), s0_symbol.size(), c0);

    // The inverse of the coefficient.
    const auto inv1 = gf.divide(1, c1);

    // First, compute its size.
    const auto src_size = gf.multiply(inv1, static_cast<std::uint32_t>(r0.size()));
    REQUIRE(src_size == s1_symbol.size());

    // Now, reconstruct missing symbol.
    detail::source s1{0, detail::byte_buffer{'x','x','x','x','x'}, src_size};
    gf.multiply(r0.buffer().data(), s1.buffer().data(), s1_symbol.size(), inv1);
    REQUIRE(s1.buffer() == s1_symbol);
  }
}

/*------------------------------------------------------------------------------------------------*/
