#include "netcode/c/decoder.h"

/*------------------------------------------------------------------------------------------------*/

ntc_decoder_t*
ntc_new_decoder( ntc_configuration_t* conf, ntc_packet_handler packet_handler
               , ntc_data_handler data_handler)
{
  return new ntc_decoder_t{ ntc::detail::c_packet_handler{packet_handler}
                          , ntc::detail::c_data_handler{data_handler}, *conf};
}

/*------------------------------------------------------------------------------------------------*/

void
ntc_delete_decoder(ntc_decoder_t* dec)
{
  delete dec;
}

/*------------------------------------------------------------------------------------------------*/

void
ntc_decoder_notify_packet(ntc_decoder_t* dec, const char* packet, size_t len)
{
  (*dec)(packet, len);
}

/*------------------------------------------------------------------------------------------------*/

void
ntc_decoder_generate_ack(ntc_decoder_t* dec)
{
  dec->generate_ack();
}

/*------------------------------------------------------------------------------------------------*/

ntc_configuration_t*
ntc_decoder_get_configuration(ntc_decoder_t* dec)
{
  return &dec->conf();
}

/*------------------------------------------------------------------------------------------------*/
