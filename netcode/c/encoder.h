#pragma once

#ifdef __cplusplus
#include "netcode/c/detail/types.hh"
#endif

#include "netcode/c/data.h"
#include "netcode/c/error.h"
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
typedef enum {ntc_systematic, ntc_non_systematic} ntc_code_type;

/*------------------------------------------------------------------------------------------------*/

/// @ingroup c_encoder
/// @return A new decoder if allocation suceeded; a null pointer otherwise.
ntc_encoder_t*
ntc_new_encoder(uint8_t galois_field_size, ntc_packet_handler handler)
noexcept;

/*------------------------------------------------------------------------------------------------*/

/// @ingroup c_encoder
void
ntc_delete_encoder(ntc_encoder_t* enc)
noexcept;

/*------------------------------------------------------------------------------------------------*/

/// @ingroup c_encoder
/// @pre @ref ntc_data_get_used_bytes (@p data) > 0
/// @pre @p error != NULL
void
ntc_encoder_add_data(ntc_encoder_t* enc, ntc_data_t* data, ntc_error* error)
noexcept;

/*------------------------------------------------------------------------------------------------*/

/// @ingroup c_encoder
/// @pre @p error != NULL
size_t
ntc_encoder_add_packet(ntc_encoder_t* enc, const char* packet, size_t max_size, ntc_error* error)
noexcept;

/*------------------------------------------------------------------------------------------------*/

/// @ingroup c_encoder
void
ntc_encoder_generate_repair(ntc_encoder_t* enc, ntc_error* error)
noexcept;

/*------------------------------------------------------------------------------------------------*/

/// @ingroup c_encoder
size_t
ntc_encoder_window(ntc_encoder_t* enc)
noexcept;

/*------------------------------------------------------------------------------------------------*/

/// @ingroup c_encoder
void
ntc_encoder_set_code_type(ntc_encoder_t* enc, ntc_code_type code_type)
noexcept;

/*------------------------------------------------------------------------------------------------*/

/// @ingroup c_encoder
/// @pre @p rate > 0
/// @note If the adaptive mode is set, the rate will change automatically.
void
ntc_encoder_set_rate(ntc_encoder_t* enc, size_t rate)
noexcept;

/*------------------------------------------------------------------------------------------------*/

/// @ingroup c_encoder
/// @brief Configure the maximal number of data to keep before discarding oldest data.
/// @pre @p size > 0
void
ntc_encoder_set_window_size(ntc_encoder_t* enc, size_t size)
noexcept;

/*------------------------------------------------------------------------------------------------*/

/// @ingroup c_encoder
/// @brief Configure the in-adaptive mode.
void
ntc_encoder_set_adaptive(ntc_encoder_t* enc, bool adaptive)
noexcept;

/*------------------------------------------------------------------------------------------------*/

#ifdef __cplusplus
} // extern "C"
#endif
