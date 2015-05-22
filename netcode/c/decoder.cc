#include <new> // nothrow
#include <stdexcept>

#include "netcode/c/decoder.h"
#include "netcode/errors.hh"

/*------------------------------------------------------------------------------------------------*/

ntc_decoder_t*
ntc_new_decoder( ntc_configuration_t* conf, ntc_packet_handler packet_handler
               , ntc_data_handler data_handler)
noexcept
{
  return new (std::nothrow) ntc_decoder_t{ ntc::detail::c_packet_handler{packet_handler}
                                         , ntc::detail::c_data_handler{data_handler}, *conf};
}

/*------------------------------------------------------------------------------------------------*/

void
ntc_delete_decoder(ntc_decoder_t* dec)
noexcept
{
  delete dec;
}

/*------------------------------------------------------------------------------------------------*/

ntc_error
ntc_decoder_notify_packet(ntc_decoder_t* dec, const char* packet, size_t max_len)
noexcept
{
  try
  {
    (*dec)(packet, max_len);
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
ntc_decoder_generate_ack(ntc_decoder_t* dec)
noexcept
{
  try
  {
    dec->generate_ack();
    return ntc_no_error;
  }
  catch (const std::exception&)
  {
    return ntc_unknown_error;
  }
}

/*------------------------------------------------------------------------------------------------*/

ntc_configuration_t*
ntc_decoder_get_configuration(ntc_decoder_t* dec)
noexcept
{
  return &dec->conf();
}

/*------------------------------------------------------------------------------------------------*/
