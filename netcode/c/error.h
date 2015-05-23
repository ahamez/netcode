#pragma once

/*------------------------------------------------------------------------------------------------*/

/// @ingroup c_error
/// @brief Describe possible errors reported by the library.
typedef enum { ntc_no_error
             , ntc_unknown_error
             , ntc_no_memory
             , ntc_packet_type_error
             , ntc_overflow_error
             } ntc_error_type;

/// @ingroup c_error
/// @brief Type to store reported errors, if any.
typedef struct
{
  ntc_error_type type;
  char* message;
} ntc_error;

/*------------------------------------------------------------------------------------------------*/
