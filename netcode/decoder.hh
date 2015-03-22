#pragma once

#include <memory>    // unique_ptr

#include "netcode/detail/decoder.hh"
#include "netcode/detail/handler.hh"
#include "netcode/detail/make_protocol.hh"
#include "netcode/detail/packet_type.hh"
#include "netcode/detail/repair.hh"
#include "netcode/detail/serializer.hh"
#include "netcode/detail/source.hh"
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

  /// @brief Constructor.
  template <typename Handler>
  decoder(Handler&& h, unsigned int ack_rate, code_type type, protocol prot)
    : type_{type}
    , ack_{}
    , ack_rate_{ack_rate}
    , nb_received_repairs_{0}
    , nb_received_sources_{0}
    , handler_{new detail::handler_derived<Handler>(std::forward<Handler>(h))}
    , serializer_{mk_protocol(prot, *handler_)}
    , decoder_{*handler_}
  {
    // Let's reserve some memory for the ack, it will most likely avoid memory re-allocations.
    ack_.source_ids().reserve(128);
  }

  /// @brief Constructor for a systematic decoder using the simple protocol.
  template <typename Handler>
  decoder(Handler&& h, unsigned int ack_rate)
    : decoder{ std::forward<Handler>(h)
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
        nb_received_repairs_ += 1;
        decoder_(serializer_->read_repair(data));
        break;
      }

      case detail::packet_type::source:
      {
        nb_received_sources_ += 1;

        auto src = serializer_->read_source(data);
        detail::insertion_sort(ack_.source_ids(), src.id());
        decoder_(std::move(src));
        break;
      }

      default:
      {
        return false;
      }
    }

    // Do we need to send an ack?
    /// @todo Also count decoded sources
    if ((nb_received_repairs_ + nb_received_sources_) % ack_rate_ == 0)
    {
      // Ask serializer to handle the bytes of the new ack (will be routed to user's handler).
      serializer_->write_ack(ack_);

      // Start a fresh new ack.
      ack_.reset();
    }

    return true;
  }

private:

  /// @brief Is the encoder systematic?
  code_type type_;

  /// @brief Re-use the same memory to prepare an ack packet.
  detail::ack ack_;

  /// @brief The number of source packets to receive before sending an ack packet.
  unsigned int ack_rate_;

  /// @brief The counter of received repairs.
  std::size_t nb_received_repairs_;

  /// @brief The counter of received sources.
  std::size_t nb_received_sources_;

  /// @brief The user's handler for various callbacks.
  std::unique_ptr<detail::handler_base> handler_;

  /// @brief How to serialize packets.
  std::unique_ptr<detail::serializer_base> serializer_;

  /// @brief The component that rebuilds sources using repairs.
  detail::decoder decoder_;
};

/*------------------------------------------------------------------------------------------------*/

} // namespace ntc
