#pragma once

#include "galois/field.hh"

namespace galois {

/*------------------------------------------------------------------------------------------------*/

inline
void
multiply(field& f, std::size_t len, const char* src, char* dst, uint32_t coefficient)
{
  f.multiply_region_w32(src, dst, coefficient, len, false /* dont't add in dst with xor */);
}


/*------------------------------------------------------------------------------------------------*/

inline
void
multiply_add(field& f, std::size_t len, const char* src, char* dst, uint32_t coefficient)
{
  f.multiply_region_w32(src, dst, coefficient, len, true /* add in dst with xor */);
}

/*------------------------------------------------------------------------------------------------*/

} // namespace galois
