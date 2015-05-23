#pragma once

#ifdef __cplusplus
#include "netcode/c/detail/types.hh"
#endif

#include "netcode/c/configuration.h"
#include "netcode/c/error.h"
#include "netcode/c/handlers.h"

/*------------------------------------------------------------------------------------------------*/

#ifdef __cplusplus
extern "C" {
#endif

/*------------------------------------------------------------------------------------------------*/

#ifndef __cplusplus

/// @brief The type of a decoder.
/// @ingroup c_decoder
typedef struct ntc_decoder_t ntc_decoder_t;

// Hide this C++ keyword from C
#define noexcept

#endif

/*------------------------------------------------------------------------------------------------*/

/// @ingroup c_decoder
/// @return A new decoder if allocation suceeded; a null pointer otherwise.
ntc_decoder_t*
ntc_new_decoder( ntc_configuration_t* conf, ntc_packet_handler packet_handler
               , ntc_data_handler data_handler)
               noexcept;

/*------------------------------------------------------------------------------------------------*/

/// @ingroup c_decoder
void
ntc_delete_decoder(ntc_decoder_t* dec)
noexcept;

/*------------------------------------------------------------------------------------------------*/

/// @ingroup c_decoder
size_t
ntc_decoder_notify_packet(ntc_decoder_t* dec, const char* packet, size_t max_size, ntc_error* error)
noexcept;

/*------------------------------------------------------------------------------------------------*/

/// @ingroup c_decoder
void
ntc_decoder_generate_ack(ntc_decoder_t* dec, ntc_error* error)
noexcept;

/*------------------------------------------------------------------------------------------------*/

/// @ingroup c_decoder
ntc_configuration_t*
ntc_decoder_get_configuration(ntc_decoder_t* dec)
noexcept;

/*------------------------------------------------------------------------------------------------*/

#ifdef __cplusplus
} // extern "C"
#endif
