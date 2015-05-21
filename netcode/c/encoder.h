#pragma once

#ifdef __cplusplus
#include "netcode/c/detail/types.hh"
#endif

#include "netcode/c/configuration.h"
#include "netcode/c/data.h"
#include "netcode/c/handlers.h"

#ifdef __cplusplus
extern "C" {
#endif

/*------------------------------------------------------------------------------------------------*/

#ifndef __cplusplus
/// @brief The type of an encoder.
typedef struct ntc_encoder_t ntc_encoder_t;
#endif

/*------------------------------------------------------------------------------------------------*/

ntc_encoder_t*
ntc_new_encoder(ntc_configuration_t* conf, ntc_packet_handler handler);

/*------------------------------------------------------------------------------------------------*/

void
ntc_delete_encoder(ntc_encoder_t* enc);

/*------------------------------------------------------------------------------------------------*/

void
ntc_encoder_commit_data(ntc_encoder_t* enc, ntc_data_t* data);

/*------------------------------------------------------------------------------------------------*/

void
ntc_encoder_notify_packet(ntc_encoder_t* enc, const char* packet, size_t len);

/*------------------------------------------------------------------------------------------------*/

void
ntc_encoder_send_repair(ntc_encoder_t* enc);

/*------------------------------------------------------------------------------------------------*/

size_t
ntc_encoder_window(ntc_encoder_t* enc);

/*------------------------------------------------------------------------------------------------*/

ntc_configuration_t*
ntc_encoder_get_configuration(ntc_encoder_t* enc);

/*------------------------------------------------------------------------------------------------*/

#ifdef __cplusplus
} // extern "C"
#endif
