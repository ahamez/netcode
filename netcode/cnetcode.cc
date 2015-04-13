#include "cnetcode.h"

/*------------------------------------------------------------------------------------------------*/
// Data
/*------------------------------------------------------------------------------------------------*/

ntc_data_t*
ntc_new_data(size_t sz)
{
  return new ntc_data_t{sz};
}

ntc_data_t*
ntc_new_data_copy(const char* src, size_t sz)
{
  return new ntc_data_t{src, sz};
}

void
ntc_delete_data(ntc_data_t* d)
{
  delete d;
}

char*
ntc_data_buffer(ntc_data_t* d)
{
  return d->buffer();
}

void
ntc_data_used_bytes(ntc_data_t* d, size_t sz)
{
  d->used_bytes() = sz;
}

void
ntc_data_reset(ntc_data_t* d, size_t sz)
{
  d->reset(sz);
}

/*------------------------------------------------------------------------------------------------*/
// Configuration
/*------------------------------------------------------------------------------------------------*/

ntc_configuration_t*
ntc_new_configuration()
{
  return new ntc_configuration_t{};
}

void
ntc_delete_configuration(ntc_configuration_t* conf)
{
  delete conf;
}

void
ntc_configuration_set_galois_field_size(ntc_configuration_t* conf , size_t size)
{
  conf->galois_field_size = size;
}

void
ntc_configuration_set_code_type(ntc_configuration_t* conf, enum ntc_code_type code_type)
{
  switch (code_type)
  {
    case ntc_systematic:
      conf->code_type = ntc::code::systematic;
      break;

    case ntc_non_systematic:
    default:
      conf->code_type = ntc::code::non_systematic;
  }
}

void
ntc_configuration_set_rate(ntc_configuration_t* conf, size_t rate)
{
  conf->rate = rate;
}

void
ntc_configuration_set_ack_frequency(ntc_configuration_t* conf, size_t frequency)
{
  conf->ack_frequency = std::chrono::milliseconds{frequency};
}

void
ntc_configuration_set_window(ntc_configuration_t* conf, size_t window)
{
  conf->window = window;
}

/*------------------------------------------------------------------------------------------------*/
// Encoder
/*------------------------------------------------------------------------------------------------*/

ntc_encoder_t*
ntc_new_encoder(ntc_configuration_t* conf, ntc_packet_handler handler)
{
  return new ntc_encoder_t{c_packet_handler{handler}, *conf};
}

void
ntc_delete_encoder(ntc_encoder_t* enc)
{
  delete enc;
}

ntc_configuration_t*
ntc_encoder_get_configuration(ntc_encoder_t* enc)
{
  return &enc->conf();
}

void
ntc_encoder_commit_data(ntc_encoder_t* enc, ntc_data_t* data)
{
  (*enc)(std::move(*data));
}

void
ntc_encoder_notify_packet(ntc_encoder_t* enc, const char* packet)
{
  (*enc)(packet);
}

void
ntc_encoder_end_repair(ntc_encoder_t* enc)
{
  enc->send_repair();
}

size_t
ntc_encoder_window(ntc_encoder_t* enc)
{
  return enc->window();
}

/*------------------------------------------------------------------------------------------------*/
// Decoder
/*------------------------------------------------------------------------------------------------*/

ntc_decoder_t*
ntc_new_decoder( ntc_configuration_t* conf, ntc_packet_handler packet_handler
               , ntc_data_handler data_handler)
{
  return new ntc_decoder_t{c_packet_handler{packet_handler}, c_data_handler{data_handler}, *conf};
}

void
ntc_delete_decoder(ntc_decoder_t* dec)
{
  delete dec;
}

ntc_configuration_t*
ntc_decoder_get_configuration(ntc_decoder_t* dec)
{
  return &dec->conf();
}

void
ntc_decoder_notify_packet(ntc_decoder_t* dec, const char* packet)
{
  (*dec)(packet);
}

void
ntc_decoder_send_ack(ntc_decoder_t* dec)
{
  dec->send_ack();
}

/*------------------------------------------------------------------------------------------------*/
