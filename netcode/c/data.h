#pragma once

#ifdef __cplusplus
#include <cstdint>
#include "netcode/c/detail/types.hh"
#else
#include <stdint.h>
#endif

#include "netcode/c/error.h"

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
/// @param size The maximal size of the data.
/// @attention When the data has been completely written, ntc_data_set_used_bytes() must be called
/// to indicate how much bytes were written. An assert will check this pre-condition in debug mode.
/// @ingroup c_data
ntc_data_t*
ntc_new_data(uint16_t size)
noexcept;

/*------------------------------------------------------------------------------------------------*/

/// @brief Create a data from a source which will be copied.
/// @note There's no need to call ntc_data_set_used_bytes() when creating a data with this function.
/// @ingroup c_data
ntc_data_t*
ntc_new_data_copy(const char* src, uint16_t size)
noexcept;

/*------------------------------------------------------------------------------------------------*/

/// @brief Release the memory of a data.
/// @ingroup c_data
void
ntc_delete_data(ntc_data_t* d)
noexcept;

/*------------------------------------------------------------------------------------------------*/

/// @brief Access the underlying buffer of a data.
/// @ingroup c_data
char*
ntc_data_buffer(ntc_data_t* d)
noexcept;

/*------------------------------------------------------------------------------------------------*/

/// @brief Get how much bytes were reserved for the underlying buffer.
/// @ingroup c_data
///
/// This reserved size can be set :
/// - at construction with ntc_new_data()
/// - at copy construction with ntc_new_data_copy()
/// - when resetting with ntc_data_reset()
uint16_t
ntc_data_get_reserved_size(const ntc_data_t* data)
noexcept;

/*------------------------------------------------------------------------------------------------*/

/// @brief Tell to the library how much bytes were written in a data.
/// @ingroup c_data
/// @pre @p nb <= ntc_data_get_reserved_size()
void
ntc_data_set_used_bytes(ntc_data_t* d, uint16_t nb)
noexcept;

/*------------------------------------------------------------------------------------------------*/

/// @brief Get the number of bytes which were written in a data.
/// @ingroup c_data
uint16_t
ntc_data_get_used_bytes(const ntc_data_t* d)
noexcept;

/*------------------------------------------------------------------------------------------------*/

/// @brief Reset a data for future use with a new wanted size.
/// @ingroup c_data
void
ntc_data_reset(ntc_data_t* d, uint16_t new_size, ntc_error* error)
noexcept;

/*------------------------------------------------------------------------------------------------*/

#ifdef __cplusplus
} // extern "C"
#endif
