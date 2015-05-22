#include <exception>
#include <new> // nothrow

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

void
ntc_encoder_commit_data(ntc_encoder_t* enc, ntc_data_t* data)
noexcept
{
  (*enc)(std::move(*data));
}

/*------------------------------------------------------------------------------------------------*/

int
ntc_encoder_notify_packet(ntc_encoder_t* enc, const char* packet, size_t max_len)
noexcept
{
  try
  {
    (*enc)(packet, max_len);
    return 0;
  }
  catch (const ntc::packet_type_error&)
  {
    return 1;
  }
  catch (const ntc::overflow_error&)
  {
    return 2;
  }
  catch (const std::exception&)
  {
    return -1;
  }
}

/*------------------------------------------------------------------------------------------------*/

void
ntc_encoder_generate_repair(ntc_encoder_t* enc)
noexcept
{
  enc->generate_repair();
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