#include <iostream>

#include "tests/catch.hpp"
#include "tests/netcode/common.hh"
#include "tests/netcode/launch.hh"

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
  launch([](std::uint8_t gf_size)
  {
    detail::galois_field gf{gf_size};

    // The payload that should be reconstructed.
    detail::byte_buffer s0_data{'a','b','c','d'};

    // Push some sources.
    detail::source_list sl;
    add_source(sl, 0, detail::byte_buffer{s0_data}, s0_data.size());
    REQUIRE(sl.size() == 1);

    // A repair to store encoded sources
    detail::repair r0{0 /* id */};

    // We need an encoder to fill the repair.
    detail::encoder{gf_size}(r0, sl);
    REQUIRE(*r0.source_ids().begin() == 0);

    // The inverse of the coefficient.
    const auto inv = gf.invert(gf.coefficient(0, 0));

    // s0 is lost, reconstruct it in a new source.

    // First, compute its size.
    const auto src_size = gf.multiply_size(r0.encoded_size(), inv);
    REQUIRE(src_size == 4);

    detail::source s0{0, detail::byte_buffer{'x','x','x','x'}, src_size};
    gf.multiply(r0.buffer().data(), s0.buffer().data(), src_size, inv);
    REQUIRE(s0.buffer() == s0_data);
    REQUIRE(s0.buffer().size() == src_size);
  });
}

/*------------------------------------------------------------------------------------------------*/

TEST_CASE("Encode two sizes")
{
  launch([](std::uint8_t gf_size)
  {
    detail::galois_field gf{gf_size};

    const std::uint32_t c0 = gf.coefficient(0 /*repair*/, 0 /*src*/);
    const std::uint32_t s0 = 4;

    // Initialize with size of s0.
    auto r0 = gf.multiply(c0, s0);

    const auto inv0 = gf.invert(c0);
    REQUIRE(gf.multiply(inv0, r0) == 4);


    const std::uint32_t c1 = gf.coefficient(0 /*repair*/, 1 /*src*/);
    const std::uint32_t s1 = 5;

    // Add size of s1.
    r0 ^= gf.multiply(c1, s1);

    const auto inv1 = gf.invert(c1);

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
  });
}

/*------------------------------------------------------------------------------------------------*/

TEST_CASE("Encode two sources")
{
  launch([](std::uint8_t gf_size)
  {
    detail::galois_field gf{gf_size};

    // The payloads that should be reconstructed.
    detail::byte_buffer s0_data{'a','b','c','d'};
    detail::byte_buffer s1_data{'e','f','g','h','i','j','k','l'};

    // The coefficients.
    const auto c0 = gf.coefficient(0 /*repair*/, 0 /*src*/);
    const auto c1 = gf.coefficient(0 /*repair*/, 1 /*src*/);

    // Push two sources.
    detail::source_list sl;
    const auto& s0 = add_source(sl, 0, detail::byte_buffer{s0_data}, s0_data.size());
    const auto& s1 = add_source(sl, 1, detail::byte_buffer{s1_data}, s1_data.size());
    REQUIRE(sl.size() == 2);

    // A repair to store encoded sources
    detail::repair r0{0 /* id */};

    // We need an encoder to fill the repair.
    detail::encoder{gf_size}(r0, sl);
    REQUIRE(*(r0.source_ids().begin() + 0) == 0);
    REQUIRE(*(r0.source_ids().begin() + 1) == 1);

    SECTION("s0 is lost")
    {
      // First, remove size.
      r0.encoded_size() = gf.multiply_size(static_cast<std::uint16_t>(s1_data.size()), c1)
                        ^ r0.encoded_size();

      // Second, remove data.
      gf.multiply_add(s1.buffer().data(), r0.buffer().data(), s1.user_size(), c1);

      // The inverse of the coefficient.
      const auto inv0 = gf.invert(c0);

      // First, compute its size.
      const auto src_size = gf.multiply_size(r0.encoded_size(), inv0);
      REQUIRE(src_size == s0_data.size());

      // Now, reconstruct missing data.
      detail::source s0_dst{1, detail::byte_buffer(src_size), src_size};
      gf.multiply(r0.buffer().data(), s0_dst.buffer().data(), src_size, inv0);
      REQUIRE(s0.buffer().size() == s0_dst.buffer().size());
      for (auto i = 0ul; i < src_size; ++i)
      {
        REQUIRE(s0.buffer()[i] == s0_dst.buffer()[i]);
      }
    }

    SECTION("s1 is lost")
    {
      // First, remove size.
      r0.encoded_size() = gf.multiply_size(static_cast<std::uint16_t>(s0_data.size()), c0)
                        ^ r0.encoded_size();
      // Second, remove data.
      gf.multiply_add(s0.buffer().data(), r0.buffer().data(), s0.user_size(), c0);

      // The inverse of the coefficient.
      const auto inv1 = gf.invert(c1);

      // First, compute its size.
      const auto src_size = gf.multiply_size(r0.encoded_size(), inv1);
      REQUIRE(src_size == s1_data.size());

      // Now, reconstruct missing data.
      detail::source s1_dst{1, detail::byte_buffer(src_size), src_size};
      gf.multiply(r0.buffer().data(), s1_dst.buffer().data(), src_size, inv1);
      REQUIRE(s1.buffer().size() == s1_dst.buffer().size());
      for (auto i = 0ul; i < src_size; ++i)
      {
        REQUIRE(s1.buffer()[i] == s1_dst.buffer()[i]);
      }
    }
  });
}

/*------------------------------------------------------------------------------------------------*/

TEST_CASE("Two sources lost")
{
  launch([](std::uint8_t gf_size)
  {
    detail::galois_field gf{gf_size};

    // The payloads that should be reconstructed.
    detail::byte_buffer s0_data{'a','b','c','d'};
    detail::byte_buffer s1_data{'e','f','g','h','i','j','k','l'};

    // Push two sources.
    detail::source_list sl;
    add_source(sl, 0, detail::byte_buffer{s0_data}, s0_data.size());
    add_source(sl, 1, detail::byte_buffer{s1_data}, s1_data.size());
    REQUIRE(sl.size() == 2);

    // Repairs to store encoded sources.
    detail::repair r0{0 /* id */};
    detail::repair r1{1 /* id */};

    // Create first repair.
    detail::encoder{gf_size}(r0, sl);
    REQUIRE(r0.source_ids().size() == 2);
    REQUIRE(*(r0.source_ids().begin() + 0) == 0);
    REQUIRE(*(r0.source_ids().begin() + 1) == 1);

    // Create second repair.
    detail::encoder{gf_size}(r1, sl);
    REQUIRE(r1.source_ids().size() == 2);
    REQUIRE(*(r1.source_ids().begin() + 0) == 0);
    REQUIRE(*(r1.source_ids().begin() + 1) == 1);

    // Oops, s0 and s1 are lost, but not r0 and r1.

    // The coefficient matrix.
    //
    //    r0  r1
    // s0  a   c
    // s1  b   d
    //
    // r0 = a*s0 + b*s1
    // r1 = c*s0 + d*s1

    detail::square_matrix mat{2};
    mat(0,0) = gf.coefficient(0 /*repair*/, 0 /*src*/);
    mat(1,0) = gf.coefficient(0 /*repair*/, 1 /*src*/);
    mat(0,1) = gf.coefficient(1 /*repair*/, 0 /*src*/);
    mat(1,1) = gf.coefficient(1 /*repair*/, 1 /*src*/);

    // Invert matrix.
    //
    //    s0  s1
    // r0  A   C
    // r1  B   D
    //
    // s0 = A*r0 + B*r1
    // s1 = C*r0 + D*r1

    detail::square_matrix inv{mat.dimension()};
    REQUIRE(not detail::invert(gf, mat, inv).first);

    // Reconstruct s0.

    // But first, compute its size.
    const std::uint16_t s0_size = gf.multiply_size(r0.encoded_size(), inv(0,0))
                                ^ gf.multiply_size(r1.encoded_size(), inv(1,0));
    REQUIRE(s0_size == s0_data.size());
    // Were to reconstruct original source
    detail::source s0{0, detail::byte_buffer{'x','x','x','x'}, s0_size};

    // Now, reconstruct the data.
    gf.multiply(r0.buffer().data(), s0.buffer().data(), s0_size, inv(0,0));
    gf.multiply_add(r1.buffer().data(), s0.buffer().data(), s0_size, inv(1,0));
    REQUIRE(s0.buffer() == s0_data);

    // Reconstruct s1.

    // But first, compute its size.
    const std::uint16_t s1_size = gf.multiply_size(r0.encoded_size(), inv(0,1))
                                ^ gf.multiply_size(r1.encoded_size(), inv(1,1));
    REQUIRE(s1_size == s1_data.size());
    // Were to reconstruct original source
    detail::source s1{0, detail::byte_buffer{'x','x','x','x','x','x','x','x'}, s1_size};

    // Now, reconstruct the data.
    gf.multiply(r0.buffer().data(), s1.buffer().data(), s1_size, inv(0,1));
    gf.multiply_add(r1.buffer().data(), s1.buffer().data(), s1_size, inv(1,1));
    REQUIRE(s1.buffer() == s1_data);
  });
}

/*------------------------------------------------------------------------------------------------*/
