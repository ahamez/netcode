#include <new> // nothrow

#include "netcode/c/detail/check_error.hh"
#include "netcode/c/encoder.h"

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
ntc_encoder_commit_data(ntc_encoder_t* enc, ntc_data_t* data, ntc_error* error)
noexcept
{
  ntc::detail::check_error([&]{(*enc)(std::move(*data));}, error);
}

/*------------------------------------------------------------------------------------------------*/

size_t
ntc_encoder_notify_packet(ntc_encoder_t* enc, const char* packet, size_t max_size, ntc_error* error)
noexcept
{
  return ntc::detail::check_error([&]{return (*enc)(packet, max_size);}, error);
}

/*------------------------------------------------------------------------------------------------*/

void
ntc_encoder_generate_repair(ntc_encoder_t* enc, ntc_error* error)
noexcept
{
  ntc::detail::check_error([&]{enc->generate_repair();}, error);
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