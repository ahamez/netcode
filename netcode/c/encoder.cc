#include "netcode/c/encoder.h"

/*------------------------------------------------------------------------------------------------*/

ntc_encoder_t*
ntc_new_encoder(ntc_configuration_t* conf, ntc_packet_handler handler)
{
  return new ntc_encoder_t{ntc::detail::c_packet_handler{handler}, *conf};
}

/*------------------------------------------------------------------------------------------------*/

void
ntc_delete_encoder(ntc_encoder_t* enc)
{
  delete enc;
}

/*------------------------------------------------------------------------------------------------*/

void
ntc_encoder_commit_data(ntc_encoder_t* enc, ntc_data_t* data)
{
  (*enc)(std::move(*data));
}

/*------------------------------------------------------------------------------------------------*/

void
ntc_encoder_notify_packet(ntc_encoder_t* enc, const char* packet)
{
  (*enc)(packet);
}

/*------------------------------------------------------------------------------------------------*/

void
ntc_encoder_send_repair(ntc_encoder_t* enc)
{
  enc->send_repair();
}

/*------------------------------------------------------------------------------------------------*/

size_t
ntc_encoder_window(ntc_encoder_t* enc)
{
  return enc->window();
}

/*------------------------------------------------------------------------------------------------*/

ntc_configuration_t*
ntc_encoder_get_configuration(ntc_encoder_t* enc)
{
  return &enc->conf();
}

/*------------------------------------------------------------------------------------------------*/