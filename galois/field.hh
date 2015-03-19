#pragma once

#include <cassert>
#include <cstddef> // size_t
#include <memory>  // unique_ptr

extern "C" {
#include <gf_complete.h>
}

namespace galois {

/*------------------------------------------------------------------------------------------------*/

/// @brief A Galois field.
class field
{
public:

  // Can't copy-construct a field.
  field(const field&) = delete;

  // Can't copy a field.
  field& operator=(const field&) = delete;

  // Can move-construct a field.
  field(field&&) = default;

  // Can move a field.
  field& operator=(field&&) = default;

  /// @brief Constructor.
  field(unsigned int w)
    : gf_{new gf_t}
    , size_{w}
  {
    if (gf_init_easy(gf_.get(), w) == 0)
    {
      throw std::runtime_error("Can't allocate galois field");
    }
  }

  /// @brief Destructor.
  ~field()
  {
    if (gf_)
    {
      gf_free(gf_.get(), 0 /* non-recursive */);
    }
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
  /// @param c The constant.
  /// @param len The size of @p src and @p dst regions.
  /// @param add If true, perform a XOR with dst.
  void
  multiply_region_w32(const char* src, char* dst, std::uint32_t coeff, std::size_t len, bool add)
  noexcept
  {
    assert(reinterpret_cast<std::size_t>(src) % 4 == 0 && "src must be aligned on 4 bytes");
    assert(reinterpret_cast<std::size_t>(dst) % 4 == 0 && "dst must be aligned on 4 bytes");
    assert(len % 8 == 0 && "len must be a multiple of 8");
    gf_.get()->multiply_region.w32( gf_.get()
                                  , const_cast<char*>(src)
                                  , dst
                                  , coeff
                                  , static_cast<int>(len)
                                  , add);
  }

private:

  /// @brief The real underlying galois field.
  std::unique_ptr<gf_t> gf_;

  /// @brief This field size.
  unsigned int size_;
};

/*------------------------------------------------------------------------------------------------*/

} // namespace galois
