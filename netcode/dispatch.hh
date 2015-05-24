#pragma once

#include "netcode/detail/packet_type.hh"
#include "netcode/detail/traits.hh"
#include "netcode/decoder.hh"
#include "netcode/encoder.hh"

namespace ntc {

/*------------------------------------------------------------------------------------------------*/

/// @brief Dispatch the processing of an incoming packet.
/// @throw packet_type_error if the type could not have been read.
/// @ingroup ntc
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
