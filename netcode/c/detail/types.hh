#pragma once

#include "netcode/configuration.hh"
#include "netcode/data.hh"
#include "netcode/decoder.hh"
#include "netcode/encoder.hh"
#include "netcode/c/detail/handlers.hh"

/*------------------------------------------------------------------------------------------------*/

using ntc_configuration_t = ntc::configuration;

/*------------------------------------------------------------------------------------------------*/

using ntc_data_t = ntc::data;

/*------------------------------------------------------------------------------------------------*/

/// @internal
/// @brief The type of a decoder for C handlers.
using ntc_decoder_t = ntc::decoder<ntc::detail::c_packet_handler, ntc::detail::c_data_handler>;

/*------------------------------------------------------------------------------------------------*/

/// @internal
/// @brief The type of an encoder for C handlers.
using ntc_encoder_t = ntc::encoder<ntc::detail::c_packet_handler>;

/*------------------------------------------------------------------------------------------------*/
