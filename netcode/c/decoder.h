#pragma once

#ifdef __cplusplus
#include "netcode/c/detail/types.hh"
#endif

#include "netcode/c/configuration.h"
#include "netcode/c/handlers.h"

#ifdef __cplusplus
extern "C" {
#endif

/*------------------------------------------------------------------------------------------------*/

#ifndef __cplusplus
/// @brief The type of a decoder.
typedef struct ntc_decoder_t ntc_decoder_t;
#endif

/*------------------------------------------------------------------------------------------------*/

ntc_decoder_t*
ntc_new_decoder( ntc_configuration_t* conf, ntc_packet_handler packet_handler
               , ntc_data_handler data_handler);

/*------------------------------------------------------------------------------------------------*/

void
ntc_delete_decoder(ntc_decoder_t* dec);

/*------------------------------------------------------------------------------------------------*/

ntc_configuration_t*
ntc_decoder_get_configuration(ntc_decoder_t* dec);

/*------------------------------------------------------------------------------------------------*/

void
ntc_decoder_notify_packet(ntc_decoder_t* dec, const char* packet);

/*------------------------------------------------------------------------------------------------*/

void
ntc_decoder_send_ack(ntc_decoder_t* dec);

/*------------------------------------------------------------------------------------------------*/

#ifdef __cplusplus
} // extern "C"
#endif
