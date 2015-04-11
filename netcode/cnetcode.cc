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
// Encoder
/*------------------------------------------------------------------------------------------------*/

ntc_encoder_t*
ntc_new_encoder(ntc_packet_handler handler)
{
  return new ntc_encoder_t{c_packet_handler{handler}};
}

void
ntc_delete_encoder(ntc_encoder_t* enc)
{
  delete enc;
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
// Encoder
/*------------------------------------------------------------------------------------------------*/

ntc_decoder_t*
ntc_new_decoder(ntc_packet_handler packet_handler, ntc_data_handler data_handler)
{
  return new ntc_decoder_t{c_packet_handler{packet_handler}, c_data_handler{data_handler}};
}

void
ntc_delete_decoder(ntc_decoder_t* dec)
{
  delete dec;
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
