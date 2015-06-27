#pragma once

#include "netcode/data.hh"
#include "netcode/decoder.hh"
#include "netcode/encoder.hh"
#include "netcode/packet.hh"
#include "netcode/c/detail/handlers.hh"

/*------------------------------------------------------------------------------------------------*/

/// @internal
using ntc_data_t = ntc::data;

/*------------------------------------------------------------------------------------------------*/

/// @internal
using ntc_packet_t = ntc::packet;

/*------------------------------------------------------------------------------------------------*/

/// @internal
/// @brief The type of a decoder for C handlers.
using ntc_decoder_t = ntc::decoder<ntc::detail::c_packet_handler, ntc::detail::c_data_handler>;

/*------------------------------------------------------------------------------------------------*/

/// @internal
/// @brief The type of an encoder for C handlers.
using ntc_encoder_t = ntc::encoder<ntc::detail::c_packet_handler>;

/*------------------------------------------------------------------------------------------------*/
