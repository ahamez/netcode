#pragma once

#ifdef __cplusplus
extern "C" {
#endif

/*------------------------------------------------------------------------------------------------*/

/// @ingroup c_handlers
/// @brief The type of the callback called each time some bytes are ready.
///
/// @p cxt is the context given when constructing a ntc_packet_handler
///
/// @p pkt points to the beginning of a bunch of bytes the decoder or the encoder wants to send
///
/// @p sz is the number of bytes to read from @p pkt
typedef void (*ntc_prepare_packet)(void*, const char* pkt, size_t sz);

/*------------------------------------------------------------------------------------------------*/

/// @ingroup c_handlers
/// @brief The type of the callback called each time a packet is completely ready to be sent.
///
/// @p cxt is the context given when constructing a ntc_packet_handler
typedef void (*ntc_send_packet)(void*);

/*------------------------------------------------------------------------------------------------*/

/// @ingroup c_handlers
/// @brief The type of the handler called by encoder and decoder each time they need to send some
/// data over the network.
///
/// @p cxt is the context given when constructing a ntc_packet_handler
typedef struct
{
  /// @brief Let user have a pointer to a context each time a callback is called.
  void* context;

  /// The the callback called each time some bytes are ready.
  ntc_prepare_packet prepare_packet;

  /// @brief The callback called each time a packet is completely ready to be sent.
  ntc_send_packet send_packet;

} ntc_packet_handler;

/*------------------------------------------------------------------------------------------------*/

/// @ingroup c_handlers
/// @brief The type of the callback called each time a data has been received or decoded by the
/// decoder.
/// @note It is guarenteed that @p data is aligned on a 16-bytes boundary
///
/// @p cxt is the context given when constructing a ntc_data_handler
///
/// @p data points to the beginning of the data received or decoded by the decoder
///
/// @p sz is the number of bytes to read from @p data
typedef void (*ntc_read_data)(void* cxt, const char* data, size_t sz);

/*------------------------------------------------------------------------------------------------*/

/// @ingroup c_handlers
/// @brief The type of the handler called by decoder each time a data has been received or decoded.
typedef struct
{
  /// @brief Let user have a pointer to a context each time a callback is called.
  void* context;

  /// @brief The callback called each time a data has been received or decoded.
  ntc_read_data read_data;

} ntc_data_handler;

/*------------------------------------------------------------------------------------------------*/

#ifdef __cplusplus
} // extern "C"
#endif
