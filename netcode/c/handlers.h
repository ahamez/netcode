#pragma once

/*------------------------------------------------------------------------------------------------*/

/// @brief The type of the callback called each time some bytes are ready.
/// @ingroup c_interfaces
typedef void (*ntc_prepare_packet)(void*, const char* data, size_t sz);

/*------------------------------------------------------------------------------------------------*/

/// @brief The type of the callback called each time a packet is completely ready to be sent.
/// @ingroup c_interfaces
typedef void (*ntc_send_packet)(void*);

/*------------------------------------------------------------------------------------------------*/

/// @brief The type of the callback called each time a data has been received or decoded by the
/// decoder.
/// @ingroup c_interfaces
typedef void (*ntc_read_data)(void*, const char* data, size_t sz);

/*------------------------------------------------------------------------------------------------*/

/// @brief The type of the handler called by encoder and decoder each time they need to send some
/// data over the network.
/// @ingroup c_interfaces
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

/// @brief The type of the handler called by decoder each time a data has been received or decoded.
/// @ingroup c_interfaces
typedef struct
{
  /// @brief Let user have a pointer to a context each time a callback is called.
  void* context;

  /// @brief The callback called each time a data has been received or decoded.
  ntc_read_data read_data;

} ntc_data_handler;

/*------------------------------------------------------------------------------------------------*/
