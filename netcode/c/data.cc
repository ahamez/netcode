#include <new> // nothrow

#include "netcode/c/detail/check_error.hh"
#include "netcode/c/data.h"

/*------------------------------------------------------------------------------------------------*/

ntc_data_t*
ntc_new_data(uint16_t size)
noexcept
{
  return new (std::nothrow) ntc_data_t{size};
}

/*------------------------------------------------------------------------------------------------*/

ntc_data_t*
ntc_new_data_copy(const char* src, uint16_t size)
noexcept
{
  return new (std::nothrow) ntc_data_t{src, size};
}

/*------------------------------------------------------------------------------------------------*/

void
ntc_delete_data(ntc_data_t* d)
noexcept
{
  delete d;
}

/*------------------------------------------------------------------------------------------------*/

char*
ntc_data_buffer(ntc_data_t* d)
noexcept
{
  return d->buffer();
}

/*------------------------------------------------------------------------------------------------*/

void
ntc_data_set_used_bytes(ntc_data_t* d, uint16_t size)
noexcept
{
  d->used_bytes() = size;
}

/*------------------------------------------------------------------------------------------------*/

void
ntc_data_reset(ntc_data_t* d, uint16_t new_size, ntc_error* error)
noexcept
{
  ntc::detail::check_error([&]{d->reset(new_size);}, error);
}

/*------------------------------------------------------------------------------------------------*/
