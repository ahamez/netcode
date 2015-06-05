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
ntc_delete_data(ntc_data_t* data)
noexcept
{
  delete data;
}

/*------------------------------------------------------------------------------------------------*/

char*
ntc_data_buffer(ntc_data_t* data)
noexcept
{
  return data->buffer();
}

/*------------------------------------------------------------------------------------------------*/

uint16_t
ntc_data_get_reserved_size(const ntc_data_t* data)
noexcept
{
  return data->reserved_size();
}

/*------------------------------------------------------------------------------------------------*/

void
ntc_data_set_used_bytes(ntc_data_t* data, uint16_t size)
noexcept
{
  data->used_bytes() = size;
}

/*------------------------------------------------------------------------------------------------*/

uint16_t
ntc_data_get_used_bytes(const ntc_data_t* d)
noexcept
{
  return d->used_bytes();
}

/*------------------------------------------------------------------------------------------------*/

void
ntc_data_reset(ntc_data_t* data, uint16_t new_size, ntc_error* error)
noexcept
{
  ntc::detail::check_error([&]{data->reset(new_size);}, error);
}

/*------------------------------------------------------------------------------------------------*/
