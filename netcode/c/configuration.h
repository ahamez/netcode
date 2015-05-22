#pragma once

#ifdef __cplusplus
#include <cstdint>
#include "netcode/c/detail/types.hh"
#else
#include <stdbool.h>
#include <stdint.h>
#endif

/*------------------------------------------------------------------------------------------------*/

#ifdef __cplusplus
extern "C" {
#endif

/*------------------------------------------------------------------------------------------------*/

#ifndef __cplusplus

/// @brief The type to configure the decoder and the encoder.
/// @ingroup c_configuration
typedef struct ntc_configuration_t ntc_configuration_t;

// Hide this C++ keyword from C
#define noexcept

#endif

/*------------------------------------------------------------------------------------------------*/

/// @ingroup c_configuration
typedef enum {ntc_systematic, ntc_non_systematic} ntc_code_type;

/*------------------------------------------------------------------------------------------------*/

/// @ingroup c_configuration
/// @brief Create a default configuration.
/// @return A new configuration if allocation suceeded; a null pointer otherwise.
ntc_configuration_t*
ntc_new_configuration(uint8_t galois_field_size) noexcept;

/*------------------------------------------------------------------------------------------------*/

/// @ingroup c_configuration
/// @brief Create a default configuration.
/// @return A new configuration if allocation suceeded; a null pointer otherwise.
ntc_configuration_t*
ntc_new_default_configuration() noexcept;

/*------------------------------------------------------------------------------------------------*/

/// @ingroup c_configuration
void
ntc_delete_configuration(ntc_configuration_t* conf) noexcept;

/*------------------------------------------------------------------------------------------------*/

/// @ingroup c_configuration
void
ntc_configuration_set_code_type(ntc_configuration_t* conf, ntc_code_type code_type) noexcept;

/*------------------------------------------------------------------------------------------------*/

/// @ingroup c_configuration
/// @pre @p rate > 0
void
ntc_configuration_set_rate(ntc_configuration_t* conf, size_t rate) noexcept;

/*------------------------------------------------------------------------------------------------*/

/// @ingroup c_configuration
void
ntc_configuration_set_ack_frequency(ntc_configuration_t* conf, size_t frequency) noexcept;

/*------------------------------------------------------------------------------------------------*/

/// @ingroup c_configuration
/// @pre @p nb > 0
void
ntc_configuration_set_ack_nb_packets(ntc_configuration_t* conf, uint16_t nb) noexcept;

/*------------------------------------------------------------------------------------------------*/

/// @ingroup c_configuration
/// @pre @p sz > 0
void
ntc_configuration_set_window_size(ntc_configuration_t* conf, size_t window) noexcept;

/*------------------------------------------------------------------------------------------------*/

/// @ingroup c_configuration
void
ntc_configuration_set_in_order(ntc_configuration_t* conf, bool in_order) noexcept;

/*------------------------------------------------------------------------------------------------*/

/// @ingroup c_configuration
void
ntc_configuration_set_adaptive(ntc_configuration_t* conf, bool adaptive) noexcept;

/*------------------------------------------------------------------------------------------------*/

#ifdef __cplusplus
} // extern "C"
#endif

/*------------------------------------------------------------------------------------------------*/
