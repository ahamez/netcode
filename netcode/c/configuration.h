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
typedef struct ntc_configuration_t ntc_configuration_t;
#endif

/*------------------------------------------------------------------------------------------------*/

enum ntc_code_type{ntc_systematic, ntc_non_systematic};

/*------------------------------------------------------------------------------------------------*/

ntc_configuration_t*
ntc_new_configuration(uint8_t galois_field_size);

/*------------------------------------------------------------------------------------------------*/

ntc_configuration_t*
ntc_new_default_configuration();

/*------------------------------------------------------------------------------------------------*/

void
ntc_delete_configuration(ntc_configuration_t* conf);

/*------------------------------------------------------------------------------------------------*/

void
ntc_configuration_set_code_type(ntc_configuration_t* conf, enum ntc_code_type code_type);

/*------------------------------------------------------------------------------------------------*/

void
ntc_configuration_set_rate(ntc_configuration_t* conf, size_t rate);

/*------------------------------------------------------------------------------------------------*/

void
ntc_configuration_set_ack_frequency(ntc_configuration_t* conf, size_t frequency);

/*------------------------------------------------------------------------------------------------*/

void
ntc_configuration_set_window_size(ntc_configuration_t* conf, size_t window);

/*------------------------------------------------------------------------------------------------*/

void
ntc_configuration_set_in_order(ntc_configuration_t* conf, bool in_order);

/*------------------------------------------------------------------------------------------------*/

#ifdef __cplusplus
} // extern "C"
#endif

/*------------------------------------------------------------------------------------------------*/
