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
/// @brief Describe if an encoder uses a systematic code or not
typedef enum {ntc_systematic, ntc_non_systematic} ntc_code_type;

/*------------------------------------------------------------------------------------------------*/

/// @ingroup c_encoder
/// @return A new decoder if allocation suceeded; a null pointer otherwise
ntc_encoder_t*
ntc_new_encoder(uint8_t galois_field_size, ntc_packet_handler handler)
noexcept;

/*------------------------------------------------------------------------------------------------*/

/// @ingroup c_encoder
/// @brief Release the memory of an encoder
/// @param enc The encoder to delete
void
ntc_delete_encoder(ntc_encoder_t* enc)
noexcept
__attribute__((nonnull));

/*------------------------------------------------------------------------------------------------*/

/// @ingroup c_encoder
/// @brief Let an encoder handle a new data
/// @param enc The encoder to notify
/// @param data The data to add
/// @param error The reported error, if any
/// @pre @ref ntc_data_get_used_bytes (@p data) > 0
/// @post @p data is invalid
/// @note It's possible to put @p data back in an usable state by calling ntc_data_reset()
void
ntc_encoder_add_data(ntc_encoder_t* enc, ntc_data_t* data, ntc_error* error)
noexcept
__attribute__((nonnull));

/*------------------------------------------------------------------------------------------------*/

/// @ingroup c_encoder
/// @brief Notify an encoder with a new incoming packet
/// @param enc The encoder to notify
/// @param packet The pointer to the beginning of the incoming packet
/// @param max_size The maximal number of bytes the encoder is allowed to read from @p packet
/// @param error The reported error, if any
/// @return The number of read bytes from packet
/// @note The returned value is invalid if an error occurred
size_t
ntc_encoder_add_packet(ntc_encoder_t* enc, const char* packet, size_t max_size, ntc_error* error)
noexcept
__attribute__((nonnull));

/*------------------------------------------------------------------------------------------------*/

/// @ingroup c_encoder
/// @brief Force an encoder to generate a repair
/// @param enc The encoder to force
/// @param error The reported error, if any
void
ntc_encoder_generate_repair(ntc_encoder_t* enc, ntc_error* error)
noexcept
__attribute__((nonnull));

/*------------------------------------------------------------------------------------------------*/

/// @ingroup c_encoder
/// @brief Get the current number of data an encoder still holds
/// @param enc The encoder to query
/// @return The queried size
size_t
ntc_encoder_window(ntc_encoder_t* enc)
noexcept
__attribute__((nonnull));

/*------------------------------------------------------------------------------------------------*/

/// @ingroup c_encoder
/// @brief Configure an encoder to be systematic or not
/// @param enc The encoder to configure
/// @param code_type The code type
/// @note The default mode of an encoder is systematic
void
ntc_encoder_set_code_type(ntc_encoder_t* enc, ntc_code_type code_type)
noexcept
__attribute__((nonnull));

/*------------------------------------------------------------------------------------------------*/

/// @ingroup c_encoder
/// @brief Configure the code rate of an encoder
/// @param enc The encoder to configure
/// @param rate The desired rate
/// @pre @p rate > 0
/// @note If the adaptive mode is set, the rate will change automatically
void
ntc_encoder_set_rate(ntc_encoder_t* enc, size_t rate)
noexcept
__attribute__((nonnull));

/*------------------------------------------------------------------------------------------------*/

/// @ingroup c_encoder
/// @brief Configure the maximal number of data to keep before discarding oldest data
/// @param enc The encoder to configure
/// @param size The required maximal window size
/// @pre @p size > 0
void
ntc_encoder_set_window_size(ntc_encoder_t* enc, size_t size)
noexcept
__attribute__((nonnull));

/*------------------------------------------------------------------------------------------------*/

/// @ingroup c_encoder
/// @brief Configure the adaptive mode
/// @param enc The encoder to configure
/// @param adaptive Set to true if the adaptive mode is desired
/// @note An encoder is not adaptive by default
void
ntc_encoder_set_adaptive(ntc_encoder_t* enc, bool adaptive)
noexcept
__attribute__((nonnull));

/*------------------------------------------------------------------------------------------------*/

#ifdef __cplusplus
} // extern "C"
#endif
