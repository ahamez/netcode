#pragma once

#include "netcode/detail/encoder.hh"
#include "netcode/detail/packet_type.hh"
#include "netcode/detail/packetizer.hh"
#include "netcode/detail/repair.hh"
#include "netcode/detail/source.hh"
#include "netcode/detail/source_list.hh"
#include "netcode/configuration.hh"
#include "netcode/code.hh"
#include "netcode/data.hh"

namespace ntc {

/*------------------------------------------------------------------------------------------------*/

/// @brief The class to interact with on the sender side.
template <typename PacketHandler, typename Packetizer = detail::packetizer<PacketHandler>>
class encoder final
{
private:

  /// @brief The type of the component that de/serializes packets.
  using packetizer_type = Packetizer;

public:

  /// @brief The type of the handler that processes data ready to be sent on the network.
  using packet_handler_type = PacketHandler;

public:

  /// @brief Can't copy-construct an encoder.
  encoder(const encoder&) = delete;

  /// @brief Can't copy an encoder.
  encoder& operator=(const encoder&) = delete;

  /// @brief Constructor.
  /// @note The configuration is copied. If you ever need to modify the configuration on-the-fly,
  /// you should call ntc::encoder::conf() to access the stored configuration.
  template <typename PacketHandler_>
  encoder(PacketHandler_&& packet_handler, configuration conf)
    : conf_{conf}
    , current_source_id_{0}
    , current_repair_id_{0}
    , sources_{}
    , repair_{current_repair_id_}
    , packet_handler_(std::forward<PacketHandler_>(packet_handler))
    , encoder_{conf.galois_field_size}
    , packetizer_{packet_handler_}
    , nb_sent_repairs_{0ul}
    , nb_acks_{0ul}
    , nb_sent_sources_{0ul}
  {
    assert(conf_.rate > 0);
    // Let's reserve some memory for the repair, it will most likely avoid memory re-allocations.
    repair_.buffer().reserve(2048);
    // Same thing for the list of source identifiers.
    repair_.source_ids().reserve(128);
  }

  /// @brief Constructor with a default configuration.
  /// @note The configuration is copied. If you ever need to modify the configuration on-the-fly,
  /// you should call ntc::encoder::conf() to access the stored configuration.
  template <typename PacketHandler_>
  explicit encoder(PacketHandler_&& packet_handler)
    : encoder{std::forward<PacketHandler_>(packet_handler), configuration{}}
  {}

  /// @brief Give the encoder a new data.
  /// @param d The data to add.
  /// @attention Any use of the data @p d after this call will result in an undefined behavior,
  /// except for one case: calling data::reset() will put back @p d in a usable state.
  /// @todo Check gf-complete requirements on size of data which must be a multiple of w/8 (1 for
  /// w = 4 and w = 8.
  void
  operator()(data&& d)
  {
    assert(d.used_bytes() != 0 && "please use data::used_bytes()");
    commit_impl(std::move(d));
  }

  /// @brief Notify the encoder of a new incoming packet, typically from the network.
  /// @param packet The incoming packet.
  /// @return The number of bytes that have been read (0 if the packet was not decoded).
  std::size_t
  operator()(const char* packet)
  {
    return notify_impl(packet);
  }

  /// @brief The number of packets which have not been acknowledged.
  std::size_t
  window()
  const noexcept
  {
    return sources_.size();
  }

  /// @brief Get the number of received acks.
  std::size_t
  nb_received_acks()
  const noexcept
  {
    return nb_acks_;
  }

  /// @brief Get the number of sent repairs.
  std::size_t
  nb_sent_repairs()
  const noexcept
  {
    return nb_sent_repairs_;
  }

  /// @brief Get the number of sent sources.
  std::size_t
  nb_sent_sources()
  const noexcept
  {
    return nb_sent_sources_;
  }

  /// @brief Get the data handler.
  const packet_handler_type&
  packet_handler()
  const noexcept
  {
    return packet_handler_;
  }

  /// @brief Get the data handler.
  packet_handler_type&
  packet_handler()
  noexcept
  {
    return packet_handler_;
  }

  /// @brief Force the sending of a repair.
  void
  send_repair()
  {
    repair_.reset();
    mk_repair();
    packetizer_.write_repair(repair_);
  }

  /// @brief Get the code type.
  code
  code_type()
  const noexcept
  {
    return conf_.code_type;
  }

  /// @brief Set the code type.
  void
  code_type(code c)
  noexcept
  {
    conf_.code_type = c;
  }

  /// @brief Get the code rate.
  std::size_t
  code_rate()
  const noexcept
  {
    return conf_.rate;
  }

  /// @brief Set the code rate.
  void
  code_rate(std::size_t r)
  noexcept
  {
    conf_.rate = r >= 1 ? r : conf_.rate;
  }

  /// @brief Get the max window size.
  std::size_t
  window_max()
  const noexcept
  {
    return conf_.window;
  }

  /// @brief Set the max window size.
  void
  window_max(std::size_t w)
  noexcept
  {
    conf_.window = w >= 1 ? w : conf_.window;
  }

private:

  /// @brief Create a source from the given data and generate a repair if needed.
  /// @param d The data to add.
  /// @todo Handle possible allocation errors.
  void
  commit_impl(data&& d)
  {
    assert(d.used_bytes() <= d.buffer_impl().size() && "More used bytes than the buffer can hold");

    if (sources_.size() == conf_.window)
    {
      sources_.pop_front();
    }

    // Create a new source in-place at the end of the list of sources, "stealing" the data
    // buffer from d.
    const auto& insertion
      = sources_.emplace(current_source_id_, std::move(d.buffer_impl()), d.used_bytes());

    if (conf_.code_type == code::systematic)
    {
      // Ask packetizer to handle the bytes of the new source (will be routed to user's handler).
      packetizer_.write_source(insertion);
      nb_sent_sources_ += 1;
    }
    else // non_systematic code
    {
      send_repair();
    }

    /// @todo Should we generate a repair if window_size() == 1?
    if ((current_source_id_ + 1) % conf_.rate == 0)
    {
      send_repair();
    }

    current_source_id_ += 1;
  }

  /// @brief Notify the encoder that some data has been received.
  /// @return The number of bytes that have been read (0 if the packet was not decoded).
  std::size_t
  notify_impl(const char* data)
  {
    assert(data != nullptr);
    if (detail::get_packet_type(data) == detail::packet_type::ack)
    {
      nb_acks_ += 1;
      const auto res = packetizer_.read_ack(data);
      sources_.erase(begin(res.first.source_ids()), end(res.first.source_ids()));
      return res.second;
    }
    return 0ul;
  }

  /// @brief Launch the generation of a repair.
  void
  mk_repair()
  {
    // Set the identifier of the new repair (needed by the coder to generate coefficients).
    repair_.id() = current_repair_id_;

    // Create the repair packet from the list of sources.
    assert(sources_.size() > 0 && "Empty source list");
    encoder_(repair_, sources_);

    current_repair_id_ += 1;
    nb_sent_repairs_ += 1;
  }

private:

  /// @brief The configuration.
  configuration conf_;

  /// @brief The counter for source packets identifiers.
  std::uint32_t current_source_id_;

  /// @brief The counter for repair packets identifiers.
  std::uint32_t current_repair_id_;

  /// @brief The set of souces which have not yet been acknowledged.
  detail::source_list sources_;

  /// @brief Re-use the same memory to prepare a repair packet.
  detail::repair repair_;

  /// @brief The user's handler.
  packet_handler_type packet_handler_;

  /// @brief The component that handles the coding process.
  detail::encoder encoder_;

  /// @brief How to serialize packets.
  packetizer_type packetizer_;

  /// @brief The number of generated repairs.
  std::size_t nb_sent_repairs_;

  /// @brief The number of received ack.
  std::size_t nb_acks_;

  /// @brief The number of sent sources.
  std::size_t nb_sent_sources_;

};

/*------------------------------------------------------------------------------------------------*/

} // namespace ntc
