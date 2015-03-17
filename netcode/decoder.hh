#pragma once

#include <memory> // unique_ptr

#include "netcode/detail/handler.hh"
#include "netcode/detail/packet_type.hh"
#include "netcode/detail/protocol/simple.hh"
#include "netcode/detail/serializer.hh"
#include "netcode/code.hh"
#include "netcode/code_type.hh"
#include "netcode/protocol.hh"

namespace ntc {

/*------------------------------------------------------------------------------------------------*/

/// @brief The class to interact with on the receiver side.
class decoder final
{
public:

  /// @brief Can't copy-construct an encoder.
  decoder(const decoder&) = delete;

  /// @brief Can't copy an encoder.
  decoder& operator=(const decoder&) = delete;

  /// @brief Constructor
  template <typename Handler>
  decoder(Handler&& h, code&& coder, code_type type, protocol prot)
    : coder_{std::move(coder)}
    , type_{type}
    , handler_{new detail::handler_derived<Handler>(std::forward<Handler>(h))}
    , serializer_{mk_protocol(prot, *handler_)}
  {}

  /// @brief Constructor
  template <typename Handler>
  decoder(Handler&& h)
    : decoder{std::forward<Handler>(h), code{8}, code_type::systematic, protocol::simple}
  {}

  /// @brief Notify the encoder that some data has been received.
  /// @return false if the data could not have been decoded, true otherwise.
  bool
  notify(const char* data)
  {
    assert(data != nullptr);
    switch (detail::get_packet_type(data))
    {
        case detail::packet_type::repair:
        case detail::packet_type::source:
        return true;
        default:
        return false;
    }
  }

private:

  static
  std::unique_ptr<detail::serializer>
  mk_protocol(protocol p, detail::handler_base& h)
  {
    switch (p)
    {
      case protocol::simple :
        return std::unique_ptr<detail::serializer>{new detail::protocol::simple{h}};
    }
  }

  /// @brief The component that handles the coding process.
  code coder_;

  /// @brief Is the encoder systematic?
  code_type type_;

  /// @brief The user's handler for various callbacks.
  std::unique_ptr<detail::handler_base> handler_;

  /// @brief How to serialize packets.
  std::unique_ptr<detail::serializer> serializer_;
};

/*------------------------------------------------------------------------------------------------*/

} // namespace ntc
