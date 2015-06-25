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

/// @internal
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
  explicit galois_field(std::uint8_t w)
    : m_gf{}
    , m_w{w}
  {
    assert(w== 4 or w == 8 or w == 16 or w == 32);
    if (gf_init_easy(&m_gf, static_cast<int>(m_w)) == 0)
    {
      throw std::runtime_error("Can't allocate galois field");
    }
  }
#pragma GCC diagnostic pop

  /// @brief Destructor.
  ~galois_field()
  {
    gf_free(&m_gf, 0 /* non-recursive */);
  }

  /// @brief Get the size of this Galois field
  unsigned int
  size()
  const noexcept
  {
    return m_w;
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
    m_gf.multiply_region.w32( &m_gf
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
    m_gf.multiply_region.w32( &m_gf
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
    assert(  (((m_w == 4 or m_w == 8 or m_w == 16 ) and coeff < (1u << m_w)) or (m_w == 32))
          && "Invalid coefficient");

    if (size == 0 or coeff == 0)
    {
      return 0;
    }

    if (m_w <= 8) // 4 or 8
    {
      __attribute__((aligned(16))) std::uint16_t res;
      __attribute__((aligned(16))) std::uint16_t size_ = size;
      multiply( reinterpret_cast<char*>(&size_), reinterpret_cast<char*>(&res)
              , sizeof(std::uint16_t), coeff);
      return res;
    }
    else // w = 16 or 32
    {
      return static_cast<std::uint16_t>(m_gf.multiply.w32(&m_gf, size, coeff));
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
    return m_gf.multiply.w32(&m_gf, x, y);
  }

  /// @brief Invert a coeeficient.
  std::uint32_t
  invert(std::uint32_t coef)
  noexcept
  {
    assert(coef != 0);
    return m_gf.divide.w32(&m_gf, 1, coef);
  }

  /// @brief Get the coefficient for a repair and a source.
  /// @note The result is guaranted to be different from 0.
  /// @todo When w_ = 32, computation could overflow.
  std::uint32_t
  coefficient(std::uint32_t repair_id, std::uint32_t src_id)
  const noexcept
  {
    return m_w == 32
         ? (((repair_id + 1) + (src_id + 1)) * (repair_id + 1))
         : (((repair_id + 1) + (src_id + 1)) * (repair_id + 1)) % ((1 << m_w) - 1) + 1;
  }

private:

  /// @brief The real underlying galois field.
  gf_t m_gf;

  /// @brief This field size.
  std::uint8_t  m_w;
};

/*------------------------------------------------------------------------------------------------*/

}} // namespace galois::detail
