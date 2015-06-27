#pragma once

#include "netcode/decoder_fwd.hh"
#include "netcode/encoder_fwd.hh"

namespace ntc { namespace detail {

/*------------------------------------------------------------------------------------------------*/

/// @internal
/// @brief Trait to detect if a type is ntc::encoder.
template <typename T>
struct is_encoder
{
  static constexpr auto value = false;
};

/// @internal
/// @brief Trait to detect if a type is ntc::encoder.
template <typename PacketHandler>
struct is_encoder<ntc::encoder<PacketHandler>>
{
  static constexpr auto value = true;
};

/// @internal
/// @brief Trait to detect if a type is ntc::decoder.
template <typename T>
struct is_decoder
{
  static constexpr auto value = false;
};

/// @internal
/// @brief Trait to detect if a type is ntc::encoder.
template <typename PacketHandler, typename DataHandler>
struct is_decoder<ntc::decoder<PacketHandler, DataHandler>>
{
  static constexpr auto value = true;
};

/*------------------------------------------------------------------------------------------------*/

}} // namespace ntc::detail
