#pragma once

#include <type_traits>

#include "netcode/detail/packet_type.hh"
#include "netcode/decoder.hh"
#include "netcode/encoder.hh"

namespace ntc {

/*------------------------------------------------------------------------------------------------*/

namespace detail {

/// @internal
/// @brief Trait to detect if a type is ntc::encoder.
template <typename T>
struct is_encoder
{
  static constexpr auto value = false;
};

/// @internal
/// @brief Trait to detect if a type is ntc::encoder.
template <typename PacketHandler, typename Packetizer>
struct is_encoder<ntc::encoder<PacketHandler, Packetizer>>
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
template <typename PacketHandler, typename DataHandler, typename Packetizer>
struct is_decoder<ntc::decoder<PacketHandler, DataHandler, Packetizer>>
{
  static constexpr auto value = true;
};

} // namespace detail

/*------------------------------------------------------------------------------------------------*/

/// @brief Dispatch the processing of an incoming packet.
/// @throw packet_format_error if the type could not have been read.
template <typename Encoder, typename Decoder>
std::size_t
dispatch(Encoder& encoder, Decoder& decoder, const char* packet, std::size_t len)
{
  static_assert(detail::is_encoder<Encoder>::value, "parameter encoder is not a ntc::encoder");
  static_assert(detail::is_decoder<Decoder>::value, "parameter decoder is not a ntc::decoder");

  switch (ntc::detail::get_packet_type(packet))
  {
    case ntc::detail::packet_type::ack:
    {
      return encoder(packet, len);
    }

    case ntc::detail::packet_type::repair:
    case ntc::detail::packet_type::source:
    {
      return decoder(packet, len);
    }

    default:
      assert(false);
      __builtin_unreachable();
    ;
  }
}

/*------------------------------------------------------------------------------------------------*/

} // namespace ntc
