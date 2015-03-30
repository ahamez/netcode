#pragma once

#include <chrono>

#include "netcode/detail/decoder.hh"
#include "netcode/detail/packet_type.hh"
#include "netcode/detail/packetizer.hh"
#include "netcode/detail/repair.hh"
#include "netcode/detail/source.hh"
#include "netcode/code.hh"
#include "netcode/configuration.hh"
#include "netcode/packet.hh"

namespace ntc {

/*------------------------------------------------------------------------------------------------*/

/// @brief The class to interact with on the receiver side.
template <typename DataHandler, typename SymbolHandler>
class decoder final
{
public:

  /// @brief The type of the handler that processes data ready to be sent on the network.
  using data_handler_type = DataHandler;

  /// @brief The type of the handler that processes decoded or received symbols.
  using symbol_handler_type = SymbolHandler;

public:

  /// @brief Can't copy-construct an encoder.
  decoder(const decoder&) = delete;

  /// @brief Can't copy an encoder.
  decoder& operator=(const decoder&) = delete;

  /// @brief Constructor.
  decoder( const data_handler_type& data_handler, const symbol_handler_type& symbol_handler
         , configuration conf)
    : conf_{conf}
    , last_ack_date_(std::chrono::steady_clock::now())
    , ack_{}
    , decoder_{conf.galois_field_size, [this](const detail::source& src){handle_source(src);}}
    , data_handler_{data_handler}
    , symbol_handler_{symbol_handler}
    , packetizer_{data_handler_}
    , nb_received_repairs_{0}
    , nb_received_sources_{0}
    , nb_handled_sources_{0}
    , nb_sent_ack_{0}
  {
    // Let's reserve some memory for the ack, it will most likely avoid memory re-allocations.
    ack_.source_ids().reserve(128);
  }

  /// @brief Constructor with a default configuration.
  decoder(const data_handler_type& data_handler, const symbol_handler_type& symbol_handler)
    : decoder{data_handler, symbol_handler, configuration{}}
  {}

  /// @brief Notify the encoder of a new incoming packet.
  /// @param p The incoming packet.
  /// @return false if the data could not have been decoded, true otherwise.
  /// @attention Any use of the packet @p p after this call will result in an undefined behavior.
  bool
  operator()(packet&& p)
  {
    return notify_impl(p.buffer());
  }

  /// @brief Notify the encoder of a new incoming packet.
  /// @param p The incoming packet.
  /// @return false if the data could not have been decoded, true otherwise.
  /// @attention Any use of the packet @p p after this call will result in an undefined behavior.
  bool
  operator()(auto_packet&& p)
  {
    return notify_impl(p.buffer_.data());
  }

  /// @brief Notify the encoder of a new incoming packet.
  /// @param p The incoming packet.
  /// @return false if the data could not have been decoded, true otherwise.
  /// @attention Any use of the packet @p p after this call will result in an undefined behavior.
  bool
  operator()(copy_packet&& p)
  {
    return notify_impl(p.buffer_.data());
  }

  /// @brief Get the data handler.
  const data_handler_type&
  data_handler()
  const noexcept
  {
    return data_handler_;
  }

  /// @brief Get the data handler.
  data_handler_type&
  data_handler()
  noexcept
  {
    return data_handler_;
  }

  /// @brief Get the symbol handler.
  const symbol_handler_type&
  symbol_handler()
  const noexcept
  {
    return symbol_handler_;
  }

  /// @brief Get the symbol handler.
  symbol_handler_type&
  symbol_handler()
  noexcept
  {
    return symbol_handler_;
  }

  /// @brief The number of received repairs.
  std::size_t
  nb_received_repairs()
  const noexcept
  {
    return nb_received_repairs_;
  }

  /// @brief The number of received sources.
  std::size_t
  nb_received_sources()
  const noexcept
  {
    return nb_received_sources_;
  }

  /// @brief The number of decoded sources.
  std::size_t
  nb_decoded_sources()
  const noexcept
  {
    return nb_handled_sources_ - nb_received_sources_;
  }

  /// @brief The number of sent ack.
  std::size_t
  nb_sent_ack()
  const noexcept
  {
    return nb_sent_ack_;
  }

  /// @brief Force the sending of an ack.
  void
  send_ack()
  {
    // Ask packetizer to handle the bytes of the new ack (will be routed to user's handler).
    packetizer_.write_ack(ack_);
    nb_sent_ack_ +=1;

    // Start a fresh new ack.
    ack_.reset();
  }

  /// @brief Send an ack if needed.
  void
  maybe_ack()
  {
    if (conf_.ack_frequency != std::chrono::milliseconds{0})
    {
      // Do we need to send an ack?
      const auto now = std::chrono::steady_clock::now();
      if ((now - last_ack_date_) >= conf_.ack_frequency)
      {
        send_ack();
        last_ack_date_ = now;
      }
    }
  }

private:

  /// @brief Notify the encoder that some data has been received.
  /// @return false if the data could not have been read, true otherwise.
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
        decoder_(packetizer_.read_repair(data));
        return true;
      }

      case detail::packet_type::source:
      {
        nb_received_sources_ += 1;
        // Give the decoder this received source.
        decoder_(packetizer_.read_source(data));
        return true;
      }

      default: return false;
    }
  }

  /// @brief Callback given to the real encoder to be notify when a source is processed.
  void
  handle_source(const detail::source& src)
  {
    nb_handled_sources_ += 1;

    // Ask user to read the bytes of this new source.
    symbol_handler_(src.buffer().data(), src.user_size());

    // Send an ack if necessary.
    maybe_ack(src.id());
  }

  /// @brief Send an ack if needed.
  void
  maybe_ack(std::uint32_t src_id)
  {
    ack_.source_ids().insert(src_id);
    maybe_ack();
  }

private:

  /// @brief The configuration.
  configuration conf_;

  /// @brief The last time an ack was sent.
  std::chrono::steady_clock::time_point last_ack_date_;

  /// @brief Re-use the same memory to prepare an ack packet.
  detail::ack ack_;

  /// @brief The component that rebuilds sources using repairs.
  detail::decoder decoder_;

  /// @brief The user's handler to output data on network.
  data_handler_type data_handler_;

  /// @brief The user's handler to read decoded symbols.
  symbol_handler_type symbol_handler_;

  /// @brief How to serialize packets.
  detail::packetizer<data_handler_type> packetizer_;

  /// @brief The counter of received repairs.
  std::size_t nb_received_repairs_;

  /// @brief The counter of received sources.
  std::size_t nb_received_sources_;

  /// @brief The counter of decoded sources.
  std::size_t nb_handled_sources_;

  /// @brief The number of ack sent back to the encoder.
  std::size_t nb_sent_ack_;
};

/*------------------------------------------------------------------------------------------------*/

} // namespace ntc
