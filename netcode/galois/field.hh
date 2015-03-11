#pragma once

#include <cassert>
#include <cstddef> // size_t

extern "C" {
#include <gf_complete.h>
}

namespace galois {

/*------------------------------------------------------------------------------------------------*/

class field
{
public:

  /// @brief Constructor.
  field(unsigned int w)
    : gf_{}
  {
//    if (gf_init_easy(&gf_, w) == 0)
//    {
//      throw std::runtime_error("Can't allocate galois field");
//    }
  }

  /// @brief Destructor.
  ~field()
  {
//    gf_free(&gf_, 1 /* recursive */);
  }

  /// @brief Multiply a region with a constant.
  /// @param src The region to multiply.
  /// @param dst Where to put the result.
  /// @param c The constant.
  /// @param len The size of @p src and @p dst regions.
  /// @param add If true, perform a XOR with dst.
  void
  multiply_region_w32(const char* src, char* dst, uint32_t c, std::size_t len, bool add)
  {
    assert(reinterpret_cast<std::size_t>(src) % 4 == 0 && "src must be aligned on 4 bytes");
    assert(reinterpret_cast<std::size_t>(dst) % 4 == 0 && "dst must be aligned on 4 bytes");
    assert(len % 8 == 0 && "len must be a multiple of 8");
    gf_.multiply_region.w32( &gf_
                           , const_cast<char*>(src)
                           , dst
                           , c
                           , static_cast<int>(len)
                           , add);
  }

private:

  /// @brief The real underlying galois field.
  gf_t gf_;
};

/*------------------------------------------------------------------------------------------------*/

} // namespace galois
