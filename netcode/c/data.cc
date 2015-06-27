#include <new> // nothrow

#include "netcode/c/detail/check_error.hh"
#include "netcode/c/data.h"

/*------------------------------------------------------------------------------------------------*/

ntc_data_t*
ntc_new_data(uint16_t size)
noexcept
{
  return new (std::nothrow) ntc_data_t(size);
}

/*------------------------------------------------------------------------------------------------*/

ntc_data_t*
ntc_new_data_from(const char* src, uint16_t size)
noexcept
{
  return new (std::nothrow) ntc_data_t(src, src + size);
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
  return data->data();
}

/*------------------------------------------------------------------------------------------------*/

uint16_t
ntc_data_get_size(const ntc_data_t* data)
noexcept
{
  return static_cast<uint16_t>(data->size());
}

/*------------------------------------------------------------------------------------------------*/

void
ntc_data_resize(ntc_data_t* data, uint16_t new_size, ntc_error* error)
noexcept
{
  ntc::detail::check_error([&]{data->resize(new_size);}, error);
}

/*------------------------------------------------------------------------------------------------*/
