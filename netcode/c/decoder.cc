#include <new> // nothrow

#include "netcode/c/detail/check_error.hh"
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

size_t
ntc_decoder_notify_packet(ntc_decoder_t* dec, const char* packet, size_t max_size, ntc_error* error)
noexcept
{
  return ntc::detail::check_error([&]{return (*dec)(packet, max_size);}, error);
}

/*------------------------------------------------------------------------------------------------*/

void
ntc_decoder_generate_ack(ntc_decoder_t* dec, ntc_error* error)
noexcept
{
  ntc::detail::check_error([&]{dec->generate_ack();}, error);
}

/*------------------------------------------------------------------------------------------------*/

ntc_configuration_t*
ntc_decoder_get_configuration(ntc_decoder_t* dec)
noexcept
{
  return &dec->conf();
}

/*------------------------------------------------------------------------------------------------*/
