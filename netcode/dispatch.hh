#pragma once

#include "netcode/detail/packet_type.hh"
#include "netcode/detail/traits.hh"
#include "netcode/detail/visibility.hh"
#include "netcode/decoder.hh"
#include "netcode/encoder.hh"
#include "netcode/packet.hh"

namespace ntc {

/*------------------------------------------------------------------------------------------------*/

/// @brief Dispatch the processing of an incoming packet.
/// @throw packet_type_error if the type could not have been read.
/// @ingroup ntc
template <typename Encoder, typename Decoder>
NTC_PUBLIC
std::size_t
dispatch(Encoder& encoder, Decoder& decoder, packet&& p)
{
  static_assert(detail::is_encoder<Encoder>::value, "parameter encoder is not a ntc::encoder");
  static_assert(detail::is_decoder<Decoder>::value, "parameter decoder is not a ntc::decoder");

  switch (ntc::detail::get_packet_type(p))
  {
    case ntc::detail::packet_type::ack:
    {
      return encoder(std::move(p));
    }

    case ntc::detail::packet_type::repair:
    case ntc::detail::packet_type::source:
    {
      return decoder(std::move(p));
    }

    default:
      assert(false);
      __builtin_unreachable();
    ;
  }
}

/*------------------------------------------------------------------------------------------------*/

} // namespace ntc
