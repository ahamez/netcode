#include <exception>
#include <new> // nothrow

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

int
ntc_decoder_notify_packet(ntc_decoder_t* dec, const char* packet, size_t max_len)
noexcept
{
  try
  {
    (*dec)(packet, max_len);
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
ntc_decoder_generate_ack(ntc_decoder_t* dec)
noexcept
{
  dec->generate_ack();
}

/*------------------------------------------------------------------------------------------------*/

ntc_configuration_t*
ntc_decoder_get_configuration(ntc_decoder_t* dec)
noexcept
{
  return &dec->conf();
}

/*------------------------------------------------------------------------------------------------*/
