#include <new> // nothrow
#include <stdexcept>

#include "netcode/c/data.h"

/*------------------------------------------------------------------------------------------------*/

ntc_data_t*
ntc_new_data(uint16_t sz)
noexcept
{
  return new (std::nothrow) ntc_data_t{sz};
}

/*------------------------------------------------------------------------------------------------*/

ntc_data_t*
ntc_new_data_copy(const char* src, uint16_t sz)
noexcept
{
  return new (std::nothrow) ntc_data_t{src, sz};
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
ntc_data_set_used_bytes(ntc_data_t* d, uint16_t sz)
noexcept
{
  d->used_bytes() = sz;
}

/*------------------------------------------------------------------------------------------------*/

ntc_error
ntc_data_reset(ntc_data_t* d, uint16_t sz)
noexcept
{
  try
  {
    d->reset(sz);
    return ntc_no_error;
  }
  catch (const std::bad_alloc&)
  {
    return ntc_no_memory;
  }
  catch (const std::exception&)
  {
    return ntc_unknown_error;
  }
}

/*------------------------------------------------------------------------------------------------*/
