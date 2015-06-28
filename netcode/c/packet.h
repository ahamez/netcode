#pragma once

#ifdef __cplusplus
#include <cstdint>
#include "netcode/c/detail/types.hh"
#else
#include <stdint.h>
#endif

#include "netcode/c/detail/noexcept.hh"
#include "netcode/c/error.h"

/*------------------------------------------------------------------------------------------------*/

#ifdef __cplusplus
extern "C" {
#endif

/*------------------------------------------------------------------------------------------------*/

#ifndef __cplusplus
/// @ingroup c_packet
/// @brief The type that lets user transfer packets to the encoder and decoder
typedef struct ntc_packet_t ntc_packet_t;
#endif

/*------------------------------------------------------------------------------------------------*/

/// @ingroup c_packet
/// @brief Create an uninitialized packet
/// @param size The size of the packet
ntc_packet_t*
ntc_new_packet(uint16_t size)
noexcept;

/*------------------------------------------------------------------------------------------------*/

/// @ingroup c_packet
/// @brief Create a packet from a source which will be copied
/// @param src The packet source to copy
/// @param size The number of bytes to copy from @p src
ntc_packet_t*
ntc_new_packet_from(const char* src, uint16_t size)
noexcept
__attribute__((nonnull));

/*------------------------------------------------------------------------------------------------*/

/// @ingroup c_packet
/// @brief Release the memory of a packet.
/// @param packet The packet to delete
void
ntc_delete_packet(ntc_packet_t* packet)
noexcept
__attribute__((nonnull));

/*------------------------------------------------------------------------------------------------*/

/// @ingroup c_packet
/// @brief Access the underlying buffer of a packet.
/// @param packet The packet to modify
char*
ntc_packet_buffer(ntc_packet_t* packet)
noexcept
__attribute__((nonnull));

/*------------------------------------------------------------------------------------------------*/

/// @ingroup c_packet
/// @brief Get the size of a a packet
/// @param packet The packet to query
uint16_t
ntc_packet_get_size(const ntc_packet_t* packet)
noexcept
__attribute__((nonnull));

/*------------------------------------------------------------------------------------------------*/

/// @ingroup c_packet
/// @brief Resize a packet
/// @pre @p nb > 0
///
/// - If @p new_size <= ntc_packet_get_size(), it's a constant operation
/// - If @p new_size > ntc_packet_get_size(), a reallocation and a copy might occur
void
ntc_packet_resize(ntc_packet_t* packet, uint16_t new_size, ntc_error* error)
noexcept
__attribute__((nonnull));

/*------------------------------------------------------------------------------------------------*/

#ifdef __cplusplus
} // extern "C"
#endif
