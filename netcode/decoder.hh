#pragma once

#include <algorithm> // lower_bound
#include <memory>    // unique_ptr

#include "netcode/detail/handler.hh"
#include "netcode/detail/make_protocol.hh"
#include "netcode/detail/packet_type.hh"
#include "netcode/detail/protocol/simple.hh"
#include "netcode/detail/serializer.hh"
#include "netcode/code.hh"
#include "netcode/code_type.hh"
#include "netcode/packet.hh"
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
  decoder(Handler&& h, code&& coder, unsigned int ack_rate, code_type type, protocol prot)
    : coder_{std::move(coder)}
    , ack_rate_{ack_rate}
    , type_{type}
    , ack_{}
    , received_sources_{0}
    , handler_{new detail::handler_derived<Handler>(std::forward<Handler>(h))}
    , serializer_{mk_protocol(prot, *handler_)}
  {
    // Let's reserve some memory for the ack, it will most likely avoid memory re-allocations.
    ack_.source_ids().reserve(128);
  }

  /// @brief Constructor
  template <typename Handler>
  decoder(Handler&& h, unsigned int ack_rate)
    : decoder{ std::forward<Handler>(h)
             , code{8}
             , ack_rate
             , code_type::systematic
             , protocol::simple}
  {}

  /// @brief Notify the encoder of a new incoming packet.
  /// @param p The incoming packet.
  /// @return false if the data could not have been decoded, true otherwise.
  /// @attention Any use of the packet @p p after this call will result in an undefined behavior.
  bool
  notify(packet&& p)
  {
    return notify_impl(p.buffer());
  }

  /// @brief Notify the encoder of a new incoming packet.
  /// @param p The incoming packet.
  /// @return false if the data could not have been decoded, true otherwise.
  /// @attention Any use of the packet @p p after this call will result in an undefined behavior.
  bool
  notify(auto_packet&& p)
  {
    return notify_impl(p.buffer_.data());
  }

  /// @brief Notify the encoder of a new incoming packet.
  /// @param p The incoming packet.
  /// @return false if the data could not have been decoded, true otherwise.
  /// @attention Any use of the packet @p p after this call will result in an undefined behavior.
  bool
  notify(copy_packet&& p)
  {
    return notify_impl(p.buffer_.data());
  }

  /// @brief Get the number of generated ack.
  std::size_t
  nb_acks()
  const noexcept
  {
    return nb_acks_;
  }

private:

  /// @brief Notify the encoder that some data has been received.
  /// @return false if the data could not have been decoded, true otherwise.
  bool
  notify_impl(const char* data)
  {
    assert(data != nullptr);
    switch (detail::get_packet_type(data))
    {
      case detail::packet_type::repair:
      {
        handle_incoming(serializer_->read_repair(data));
        return true;
      }

      case detail::packet_type::source:
      {
        handle_incoming(serializer_->read_source(data));
        return true;
      }

      default:
      {
        return false;
      }
    }
  }

  void
  handle_incoming(detail::repair&&)
  {

  }

  void
  handle_incoming(detail::source&& src)
  {
    // Insertion sort of the source identifier in the list of acknowledged source.
    const auto ids_end = end(ack_.source_ids());
    const auto lb = std::lower_bound(begin(ack_.source_ids()), ids_end, src.id());
    if ((lb != ids_end and *lb != src.id()) or (lb == ids_end))
    {
      // src.id() doesn't exist in ack_.source_ids(), insert it.
      ack_.source_ids().insert(lb, src.id());
    }

    // Ask user to handle the bytes of the new src.
    handler_->on_ready_symbol(src.user_size(), src.buffer().data());

    // Do we need to send an ack?
    if ((received_sources_ + 1) % ack_rate_ == 0)
    {
      // Ask serializer to handle the bytes of the new ack (will be routed to user's handler).
      serializer_->write_ack(ack_);
      nb_acks_ += 1;

      // Start a fresh new ack.
      ack_.reset();
    }
  }

private:

  /// @brief The component that handles the coding process.
  code coder_;

  /// @brief The number of source packets to receive before sending an ack packet.
  unsigned int ack_rate_;

  /// @brief Is the encoder systematic?
  code_type type_;

  /// @brief Re-use the same memory to prepare an ack packet.
  detail::ack ack_;

  /// @brief The number of sent ack packets.
  std::size_t nb_acks_;

  /// @brief The counter of received sources.
  std::size_t received_sources_;

  /// @brief The user's handler for various callbacks.
  std::unique_ptr<detail::handler_base> handler_;

  /// @brief How to serialize packets.
  std::unique_ptr<detail::serializer_base> serializer_;
};

/*------------------------------------------------------------------------------------------------*/

} // namespace ntc
