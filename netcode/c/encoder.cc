#include <new> // nothrow
#include <stdexcept>

#include "netcode/c/encoder.h"
#include "netcode/errors.hh"

/*------------------------------------------------------------------------------------------------*/

ntc_encoder_t*
ntc_new_encoder(ntc_configuration_t* conf, ntc_packet_handler handler)
noexcept
{
  return new (std::nothrow) ntc_encoder_t{ntc::detail::c_packet_handler{handler}, *conf};
}

/*------------------------------------------------------------------------------------------------*/

void
ntc_delete_encoder(ntc_encoder_t* enc)
noexcept
{
  delete enc;
}

/*------------------------------------------------------------------------------------------------*/

ntc_error
ntc_encoder_commit_data(ntc_encoder_t* enc, ntc_data_t* data)
noexcept
{
  try
  {
    (*enc)(std::move(*data));
    return ntc_no_error;
  }
  catch (const ntc::packet_type_error&)
  {
    return ntc_packet_type_error;
  }
  catch (const ntc::overflow_error&)
  {
    return ntc_overflow_error;
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

ntc_error
ntc_encoder_notify_packet(ntc_encoder_t* enc, const char* packet, size_t max_len)
noexcept
{
  try
  {
    (*enc)(packet, max_len);
    return ntc_no_error;
  }
  catch (const ntc::packet_type_error&)
  {
    return ntc_packet_type_error;
  }
  catch (const ntc::overflow_error&)
  {
    return ntc_overflow_error;
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

ntc_error
ntc_encoder_generate_repair(ntc_encoder_t* enc)
noexcept
{
  try
  {
    enc->generate_repair();
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

size_t
ntc_encoder_window(ntc_encoder_t* enc)
noexcept
{
  return enc->window();
}

/*------------------------------------------------------------------------------------------------*/

ntc_configuration_t*
ntc_encoder_get_configuration(ntc_encoder_t* enc)
noexcept
{
  return &enc->conf();
}

/*------------------------------------------------------------------------------------------------*/