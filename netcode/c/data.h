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

/// @brief The type that lets user transfer data to the encoder
/// @ingroup c_data
typedef struct ntc_data_t ntc_data_t;

// Hide this C++ keyword from C
#define noexcept

#endif

/*------------------------------------------------------------------------------------------------*/

/// @brief Create an uninitialized data to be given to the encoder
/// @param size The size of the data
/// @ingroup c_data
ntc_data_t*
ntc_new_data(uint16_t size)
noexcept;

/*------------------------------------------------------------------------------------------------*/

/// @brief Create a data from a source which will be copied
/// @ingroup c_data
/// @param src The data source to copy
/// @param size The number of bytes to copy from @p src
ntc_data_t*
ntc_new_data_from(const char* src, uint16_t size)
noexcept
__attribute__((nonnull));

/*------------------------------------------------------------------------------------------------*/

/// @brief Release the memory of a data.
/// @ingroup c_data
/// @param data The data to delete
void
ntc_delete_data(ntc_data_t* data)
noexcept
__attribute__((nonnull));

/*------------------------------------------------------------------------------------------------*/

/// @brief Access the underlying buffer of a data.
/// @ingroup c_data
/// @param data The data to modify
char*
ntc_data_buffer(ntc_data_t* data)
noexcept
__attribute__((nonnull));

/*------------------------------------------------------------------------------------------------*/

/// @brief Get the size of a a data
/// @ingroup c_data
/// @param data The data to query
uint16_t
ntc_data_get_size(const ntc_data_t* data)
noexcept
__attribute__((nonnull));

/*------------------------------------------------------------------------------------------------*/

/// @brief Resize a data
/// @ingroup c_data
/// @pre @p nb > 0
///
/// - If @p new_size <= ntc_data_get_size(), it's a constant operation
/// - If @p new_size > ntc_data_get_size(), a reallocation and a copy might occur
void
ntc_data_resize(ntc_data_t* data, uint16_t new_size, ntc_error* error)
noexcept
__attribute__((nonnull));

/*------------------------------------------------------------------------------------------------*/

#ifdef __cplusplus
} // extern "C"
#endif
