#pragma once

#ifdef __cplusplus
#include "netcode/c/detail/types.hh"
#endif

#include "netcode/c/detail/noexcept.hh"
#include "netcode/c/error.h"
#include "netcode/c/handlers.h"
#include "netcode/c/packet.h"

#ifdef __cplusplus
extern "C" {
#endif

/*------------------------------------------------------------------------------------------------*/

#ifndef __cplusplus
/// @brief The type of a decoder
/// @ingroup c_decoder
typedef struct ntc_decoder_t ntc_decoder_t;
#endif

/*------------------------------------------------------------------------------------------------*/

/// @ingroup c_decoder
/// @brief Describe if an decoder gives data in order or not
typedef enum {ntc_in_order_yes, ntc_in_order_no} ntc_ordering_type;

/*------------------------------------------------------------------------------------------------*/

/// @ingroup c_decoder
/// @return A new decoder if allocation suceeded; a null pointer otherwise
ntc_decoder_t*
ntc_new_decoder( uint8_t galois_field_size, ntc_ordering_type order
               , ntc_packet_handler packet_handler, ntc_data_handler data_handler)
noexcept;

/*------------------------------------------------------------------------------------------------*/

/// @ingroup c_decoder
/// @brief Release the memory of a decoder
/// @param dec The decoder to delete
void
ntc_delete_decoder(ntc_decoder_t* dec)
noexcept
__attribute__((nonnull));

/*------------------------------------------------------------------------------------------------*/

/// @ingroup c_decoder
/// @brief Notify an decoder with a new incoming packet
/// @param dec The decoder to notify
/// @param packet The incoming packet
/// @param error The reported error, if any
/// @return The number of read bytes from packet
/// @note The returned value is invalid if an error occurred
/// @post @p packet is invalid
/// @note It's possible to put @p packet back in an usable state by calling ntc_packet_resize()
size_t
ntc_decoder_add_packet(ntc_decoder_t* dec, ntc_packet_t* packet, ntc_error* error)
noexcept
__attribute__((nonnull));

/*------------------------------------------------------------------------------------------------*/

/// @ingroup c_decoder
/// @brief Force an decoder to generate an acknowledgment packet
/// @param dec The decoder to force
/// @param error The reported error, if any
void
ntc_decoder_generate_ack(ntc_decoder_t* dec, ntc_error* error)
noexcept
__attribute__((nonnull));

/*------------------------------------------------------------------------------------------------*/

/// @ingroup c_decoder
/// @brief Configure the frequency at which acknowledgments will be sent
/// @param dec The decoder to configure
/// @param frequency The requested frequency, if 0, deactivate this feature
void
ntc_decoder_set_ack_frequency(ntc_decoder_t* dec, size_t frequency)
noexcept
__attribute__((nonnull));

/*------------------------------------------------------------------------------------------------*/

/// @ingroup c_decoder
/// @brief Configure the number of packets to receive before sending an acknowledgment
/// @pre @p nb > 0
void
ntc_decoder_set_ack_nb_packets(ntc_decoder_t* dec, uint16_t nb)
noexcept
__attribute__((nonnull));

/*------------------------------------------------------------------------------------------------*/

#ifdef __cplusplus
} // extern "C"
#endif
