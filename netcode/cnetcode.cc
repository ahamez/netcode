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
  return new ntc_encoder_t{ntc::detail::c_handler{handler}};
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

/*------------------------------------------------------------------------------------------------*/
