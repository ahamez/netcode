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
    , nb_decoded_sources_{0}
    , handler_{new detail::handler_derived<Handler>(std::forward<Handler>(h))}
    , serializer_{mk_protocol(prot, *handler_)}
    , decoder_{[this](const detail::source& src){source_decoded(src);}}
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
        // Give the decoder this received repair.
        decoder_(serializer_->read_repair(data));
        return true;
      }

      case detail::packet_type::source:
      {
        nb_received_sources_ += 1;
        auto src = serializer_->read_source(data);
        // Ask user to read the bytes of this new source.
        handler_->on_ready_symbol(src.user_size(), src.buffer().data());
        // Send an ack if necessary.
        ack(src.id());
        // Give the decoder this received source.
        decoder_(std::move(src));
        return true;
      }

      default: return false;
    }
  }

  /// @brief Callback given to the real encoder to be notify when a source has been decoded.
  void
  source_decoded(const detail::source& src)
  {
    nb_decoded_sources_ += 1;

    // Ask user to read the bytes of this new source.
    handler_->on_ready_symbol(src.user_size(), src.buffer().data());

    // Send an ack if necessary.
    ack(src.id());
  }

  /// @brief Create and send an ack if needed.
  void
  ack(std::uint32_t src_id)
  {
    detail::insertion_sort(ack_.source_ids(), src_id);

    // Do we need to send an ack?
    if ((nb_received_repairs_ + nb_received_sources_ + nb_decoded_sources_) % ack_rate_ == 0)
    {
      // Ask serializer to handle the bytes of the new ack (will be routed to user's handler).
      serializer_->write_ack(ack_);

      // Start a fresh new ack.
      ack_.reset();
    }
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

  /// @brief The counter of decoded sources.
  std::size_t nb_decoded_sources_;

  /// @brief The user's handler for various callbacks.
  std::unique_ptr<detail::handler_base> handler_;

  /// @brief How to serialize packets.
  std::unique_ptr<detail::serializer_base> serializer_;

  /// @brief The component that rebuilds sources using repairs.
  detail::decoder decoder_;
};

/*------------------------------------------------------------------------------------------------*/

} // namespace ntc
