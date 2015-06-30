/*------------------------------------------------------------------------------------------------*/

/// @defgroup ntc C++ interfaces

/// @defgroup ntc_encoder Encoding data
/// @ingroup ntc

/// @defgroup ntc_decoder Decoding data
/// @ingroup ntc

/// @defgroup ntc_data Manipulating data
/// @ingroup ntc

/// @defgroup ntc_packets Manipulating packets
/// @ingroup ntc

/// @defgroup ntc_error Error reporting
/// @ingroup ntc

/*------------------------------------------------------------------------------------------------*/

/// @defgroup c_ntc C interfaces
/// @attention In all cases, a @p ntc_* function shall never be given a NULL pointer

/// @defgroup c_encoder Encoding data
/// @ingroup c_ntc

/// @defgroup c_decoder Decoding data
/// @ingroup c_ntc

/// @defgroup c_data Manipulating data
/// @ingroup c_ntc

/// @defgroup c_packet Manipulating packets
/// @ingroup c_ntc

/// @defgroup c_handlers Signature of handlers to interact with encoder and decoder
/// @ingroup c_ntc

/// @defgroup c_error Error reporting
/// @ingroup c_ntc
///
/// Several functions take a parameter of type ntc_error as pointer.

/*------------------------------------------------------------------------------------------------*/

/// @brief The top level namespace of the netcode library.
namespace ntc {

/// @internal
/// @brief Implementation details of the netcode library, not meant for client usage.
namespace detail {

} // namespace detail
} // namespace ntc

/*------------------------------------------------------------------------------------------------*/
