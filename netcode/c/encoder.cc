#include <new> // nothrow

#include "netcode/c/detail/check_error.hh"
#include "netcode/c/encoder.h"

/*------------------------------------------------------------------------------------------------*/

ntc_encoder_t*
ntc_new_encoder(uint8_t galois_field_size, ntc_packet_handler handler)
noexcept
{
  return new (std::nothrow) ntc_encoder_t{ galois_field_size
                                         , ntc::detail::c_packet_handler{handler}};
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
ntc_encoder_add_data(ntc_encoder_t* enc, ntc_data_t* data, ntc_error* error)
noexcept
{
  ntc::detail::check_error([&]{(*enc)(std::move(*data));}, error);
}

/*------------------------------------------------------------------------------------------------*/

size_t
ntc_encoder_add_packet(ntc_encoder_t* enc, ntc_packet_t* packet, ntc_error* error)
noexcept
{
  return ntc::detail::check_error([&]{return (*enc)(std::move(*packet));}, error);
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

void
ntc_encoder_set_code_type(ntc_encoder_t* enc, ntc_code_type code_type)
noexcept
{
  enc->set_code_type( code_type == ntc_systematic
                    ? ntc::systematic::yes
                    : ntc::systematic::no);
}

/*------------------------------------------------------------------------------------------------*/

void
ntc_encoder_set_rate(ntc_encoder_t* enc, size_t rate)
noexcept
{
  enc->set_rate(rate);
}

/*------------------------------------------------------------------------------------------------*/

void
ntc_encoder_set_window_size(ntc_encoder_t* enc, size_t size)
noexcept
{
  enc->set_window_size(size);
}

/*------------------------------------------------------------------------------------------------*/

void
ntc_encoder_set_adaptive(ntc_encoder_t* enc, bool adaptive)
noexcept
{
  enc->set_adaptive(adaptive);
}

/*------------------------------------------------------------------------------------------------*/

