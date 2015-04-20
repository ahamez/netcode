#pragma once

#include <cassert>
#include <cstddef> // size_t
#include <cstdint>
#include <stdexcept>

extern "C" {
#include <gf_complete.h>
}

namespace ntc { namespace detail {

/*------------------------------------------------------------------------------------------------*/

/// @brief A Galois field.
class galois_field
{
public:

  // Can't copy-construct a field.
  galois_field(const galois_field&) = delete;

  // Can't copy a field.
  galois_field& operator=(const galois_field&) = delete;

  // Can move-construct a field.
  galois_field(galois_field&&) = delete;

  // Can move a field.
  galois_field& operator=(galois_field&&) = delete;

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wmissing-field-initializers"
  /// @brief Constructor.
  galois_field(std::uint8_t w)
    : gf_{}
    , w_{w}
  {
    assert(w == 8 or w == 16 or w == 32);
    if (gf_init_easy(&gf_, static_cast<int>(w_)) == 0)
    {
      throw std::runtime_error("Can't allocate galois field");
    }
  }
#pragma GCC diagnostic pop

  /// @brief Destructor.
  ~galois_field()
  {
    gf_free(&gf_, 0 /* non-recursive */);
  }

  /// @brief Get the size of this Galois field
  unsigned int
  size()
  const noexcept
  {
    return w_;
  }

  /// @brief Multiply a region with a constant.
  /// @param src The region to multiply.
  /// @param dst Where to put the result.
  /// @param len The size of @p src and @p dst regions.
  /// @param coeff The constant.
  void
  multiply(const char* src, char* dst, std::size_t len, std::uint32_t coeff)
  noexcept
  {
    gf_.multiply_region.w32( &gf_
                           , const_cast<char*>(src)
                           , dst
                           , coeff
                           , static_cast<int>(len)
                           , 0 /* don't add to src */);
  }

  /// @brief Multiply a region with a constant, add the result with the source.
  /// @param src The region to multiply.
  /// @param dst Where to put the result.
  /// @param len The size of @p src and @p dst regions.
  /// @param coeff The constant.
  void
  multiply_add(const char* src, char* dst, std::size_t len, std::uint32_t coeff)
  noexcept
  {
    gf_.multiply_region.w32( &gf_
                           , const_cast<char*>(src)
                           , dst
                           , coeff
                           , static_cast<int>(len)
                           , 1 /* add to src */);
  }

  /// @brief Multiply a size with a coefficient.
  /// @attention Make sure that the coefficient is generated with galois_field::coefficient.
  std::uint16_t
  multiply_size(std::uint16_t size, std::uint32_t coeff)
  noexcept
  {
    assert((((w_ == 8 or w_ == 16 ) and coeff < (1u << w_)) or (w_ == 32)) && "Invalid coefficient");

    if (size == 0 or coeff == 0)
    {
      return 0;
    }

    if (w_ == 8)
    {
      const auto size_hi  = static_cast<std::uint16_t>(size >> 8);
      const auto coeff_hi = static_cast<std::uint16_t>(coeff >> 8);

      const auto size_lo  = static_cast<std::uint16_t>(size & (0x00FF));
      const auto coeff_lo = static_cast<std::uint16_t>(coeff & (0x00FF));

      const auto hi = static_cast<std::uint16_t>(gf_.multiply.w32(&gf_, size_hi, coeff_hi));
      const auto lo = static_cast<std::uint16_t>(gf_.multiply.w32(&gf_, size_lo, coeff_lo));

      return static_cast<std::uint16_t>((hi << 8) ^ lo);
    }
    else // w = 16 or 32
    {
      return static_cast<std::uint16_t>(gf_.multiply.w32(&gf_, size, coeff));
    }
  }

  /// @brief Multiply two coefficients, to use when inverting a matrix.
  std::uint32_t
  multiply(std::uint32_t x, std::uint32_t y)
  noexcept
  {
    if (x == 0 or y == 0)
    {
      return 0;
    }
    return gf_.multiply.w32(&gf_, x, y);
  }

  std::uint32_t
  invert(std::uint32_t coef)
  noexcept
  {
    return gf_.divide.w32(&gf_, 1, coef);
  }

  std::uint32_t
  coefficient(std::uint32_t repair_id, std::uint32_t src_id)
  noexcept
  {
    return (((repair_id + 1) + (src_id + 1)) * (repair_id + 1)) % (1 << w_);
  }

private:

  /// @brief The real underlying galois field.
  gf_t gf_;

  /// @brief This field size.
  std::uint8_t  w_;
};

/*------------------------------------------------------------------------------------------------*/

}} // namespace galois::detail
