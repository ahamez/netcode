#include <new> // nothrow

#include "netcode/c/detail/check_error.hh"
#include "netcode/c/decoder.h"
#include "netcode/errors.hh"

/*------------------------------------------------------------------------------------------------*/

ntc_decoder_t*
ntc_new_decoder( uint8_t galois_field_size, ntc_ordering_type order
               , ntc_packet_handler packet_handler, ntc_data_handler data_handler)
noexcept
{
  return new (std::nothrow) ntc_decoder_t{ galois_field_size
                                         , order == ntc_in_order ? ntc::in_order::yes
                                                                 : ntc::in_order::no
                                         , ntc::detail::c_packet_handler{packet_handler}
                                         , ntc::detail::c_data_handler{data_handler}};
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
ntc_decoder_add_packet(ntc_decoder_t* dec, const char* packet, size_t max_size, ntc_error* error)
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

void
ntc_decoder_set_ack_frequency(ntc_decoder_t* dec, size_t frequency)
noexcept
{
  dec->set_ack_frequency(std::chrono::milliseconds{frequency});
}

/*------------------------------------------------------------------------------------------------*/

void
ntc_decoder_set_ack_nb_packets(ntc_decoder_t* dec, uint16_t nb)
noexcept
{
  dec->set_ack_nb_packets(nb);
}

/*------------------------------------------------------------------------------------------------*/
