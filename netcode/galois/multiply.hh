#pragma once

#include "netcode/galois/field.hh"

namespace galois {

/*------------------------------------------------------------------------------------------------*/

inline
void
multiply(field& f, const char* src, char* dst, uint32_t coefficient, std::size_t len)
{
  f.multiply_region_w32(src, dst, coefficient, len, false /* dont't add in dst with xor */);
}


/*------------------------------------------------------------------------------------------------*/

inline
void
multiply_add(field& f, const char* src, char* dst, uint32_t coefficient, std::size_t len)
{
  f.multiply_region_w32(src, dst, coefficient, len, true /* add in dst with xor */);
}

/*------------------------------------------------------------------------------------------------*/

template <typename SourceIterator>
void
multiply_add( field& f, SourceIterator src_begin, SourceIterator src_end
            , char* dst, uint32_t coefficient, std::size_t len)
{
  multiply(f, *src_begin, dst, coefficient, len); // no need to add for the first source
  while (++src_begin != src_end)
  {
    multiply_add(f, *src_begin, dst, coefficient, len);
  }
}

/*------------------------------------------------------------------------------------------------*/

} // namespace galois
