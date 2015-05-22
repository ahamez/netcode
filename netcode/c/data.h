#pragma once

#ifdef __cplusplus
#include <cstdint>
#include "netcode/c/detail/types.hh"
#else
#include <stdint.h>
#endif

#include "netcode/c/errors.h"

/*------------------------------------------------------------------------------------------------*/

#ifdef __cplusplus
extern "C" {
#endif

/*------------------------------------------------------------------------------------------------*/

#ifndef __cplusplus

/// @brief The type that lets user transfer data to the encoder.
/// @ingroup c_data
typedef struct ntc_data_t ntc_data_t;

// Hide this C++ keyword from C
#define noexcept

#endif

/*------------------------------------------------------------------------------------------------*/

/// @brief Create an uninitialized data to be given to the encoder.
/// @param sz The maximal size of the data.
/// @attention When the data has been completely written, ntc_data_set_used_bytes() must be called
/// to indicate how much bytes were written. An assert will check this pre-condition in debug mode.
/// @ingroup c_data
ntc_data_t*
ntc_new_data(uint16_t sz) noexcept;

/*------------------------------------------------------------------------------------------------*/

/// @brief Create a data from a source which will be copied.
/// @note There's no need to call ntc_data_set_used_bytes() when creating a data with this function.
/// @ingroup c_data
ntc_data_t*
ntc_new_data_copy(const char* src, uint16_t sz) noexcept;

/*------------------------------------------------------------------------------------------------*/

/// @brief Release the memory of a data.
/// @ingroup c_data
void
ntc_delete_data(ntc_data_t* d) noexcept;

/*------------------------------------------------------------------------------------------------*/

/// @brief Access the underlying buffer of a data.
/// @ingroup c_data
char*
ntc_data_buffer(ntc_data_t* d) noexcept;

/*------------------------------------------------------------------------------------------------*/

/// @brief Tell to the library how much bytes were written in a data.
/// @ingroup c_data
void
ntc_data_set_used_bytes(ntc_data_t* d, uint16_t sz) noexcept;

/*------------------------------------------------------------------------------------------------*/

/// @brief Reset a data for future use (to avoid a memory allocation).
/// @ingroup c_data
ntc_error
ntc_data_reset(ntc_data_t* d, uint16_t sz) noexcept;

/*------------------------------------------------------------------------------------------------*/

#ifdef __cplusplus
} // extern "C"
#endif
