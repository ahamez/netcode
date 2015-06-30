#pragma once

#ifdef __cplusplus
extern "C" {
#endif

/*------------------------------------------------------------------------------------------------*/

/// @ingroup c_error
/// @brief Describe possible errors reported by the library
typedef enum {
               ntc_no_error            ///< No error to report
             , ntc_unknown_error       ///< An unknown error occurred
             , ntc_no_memory           ///< The library was unabled to allocate more memory
             , ntc_packet_type_error   ///< The wrong type of packet has been given when calling
                                       ///  ntc_encoder_add_packet() or ntc_decoder_add_packet()
             , ntc_overflow_error      ///< The library read more bytes than it was allowed to
                                       ///  when calling ntc_encoder_add_packet() or
                                       ///  ntc_decoder_add_packet()
             } ntc_error_type;

/// @ingroup c_error
/// @brief Type to store reported errors, if any
typedef struct
{
  /// @brief The type of the reported error
  ntc_error_type type;

  /// @brief A message describing the error
  /// @note Might be NULL
  char* message;

} ntc_error;

/*------------------------------------------------------------------------------------------------*/

#ifdef __cplusplus
} // extern "C"
#endif
