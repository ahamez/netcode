#pragma once

#ifdef __cplusplus
#include "netcode/c/detail/types.hh"
#endif

#include "netcode/c/configuration.h"
#include "netcode/c/data.h"
#include "netcode/c/errors.h"
#include "netcode/c/handlers.h"

/*------------------------------------------------------------------------------------------------*/

#ifdef __cplusplus
extern "C" {
#endif

/*------------------------------------------------------------------------------------------------*/

#ifndef __cplusplus

/// @brief The type of an encoder.
/// @ingroup c_encoder
typedef struct ntc_encoder_t ntc_encoder_t;

// Hide this C++ keyword from C
#define noexcept

#endif

/*------------------------------------------------------------------------------------------------*/

/// @ingroup c_encoder
ntc_encoder_t*
ntc_new_encoder(ntc_configuration_t* conf, ntc_packet_handler handler) noexcept;

/*------------------------------------------------------------------------------------------------*/

/// @ingroup c_encoder
void
ntc_delete_encoder(ntc_encoder_t* enc) noexcept;

/*------------------------------------------------------------------------------------------------*/

/// @ingroup c_encoder
ntc_error
ntc_encoder_commit_data(ntc_encoder_t* enc, ntc_data_t* data) noexcept;

/*------------------------------------------------------------------------------------------------*/

/// @ingroup c_encoder
ntc_error
ntc_encoder_notify_packet(ntc_encoder_t* enc, const char* packet, size_t max_len) noexcept;

/*------------------------------------------------------------------------------------------------*/

/// @ingroup c_encoder
ntc_error
ntc_encoder_generate_repair(ntc_encoder_t* enc) noexcept;

/*------------------------------------------------------------------------------------------------*/

/// @ingroup c_encoder
size_t
ntc_encoder_window(ntc_encoder_t* enc) noexcept;

/*------------------------------------------------------------------------------------------------*/

/// @ingroup c_encoder
ntc_configuration_t*
ntc_encoder_get_configuration(ntc_encoder_t* enc) noexcept;

/*------------------------------------------------------------------------------------------------*/

/// @}

#ifdef __cplusplus
} // extern "C"
#endif
