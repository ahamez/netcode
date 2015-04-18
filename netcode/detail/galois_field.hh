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
  galois_field(std::size_t w)
    : gf_{}
    , size_{static_cast<std::uint32_t>(w)}
  {
    if (gf_init_easy(&gf_, static_cast<int>(size_)) == 0)
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
    return size_;
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

  std::uint32_t
  multiply(std::uint32_t x, std::uint32_t y)
  noexcept
  {
    return x == 0 or y == 0
         ? 0
         : gf_.multiply.w32(&gf_, x, y);
  }

  std::uint32_t
  divide(std::uint32_t x, std::uint32_t y)
  noexcept
  {
    return x == 0
         ? 0
         : gf_.divide.w32(&gf_, x, y);
  }

private:

  /// @brief The real underlying galois field.
  gf_t gf_;

  /// @brief This field size.
  std::uint32_t size_;
};

/*------------------------------------------------------------------------------------------------*/

}} // namespace galois::detail

//#pragma GCC diagnostic pop
