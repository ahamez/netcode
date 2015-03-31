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
#include "netcode/packet.hh"

namespace ntc {

/*------------------------------------------------------------------------------------------------*/

/// @brief The class to interact with on the sender side.
/// @todo Check gf-complete requirements on size of data which must be a multiple of w/8 (1 for
/// w = 4 and w = 8.
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
  encoder(const packet_handler_type& packet_handler, configuration conf)
    : conf_{conf}
    , current_source_id_{0}
    , current_repair_id_{0}
    , sources_{}
    , repair_{current_repair_id_}
    , packet_handler_(packet_handler)
    , encoder_{conf.galois_field_size}
    , packetizer_{packet_handler_}
    , nb_repairs_{0ul}
  {
    assert(conf_.rate > 0);
    // Let's reserve some memory for the repair, it will most likely avoid memory re-allocations.
    repair_.buffer().reserve(2048);
    // Same thing for the list of source identifiers.
    repair_.source_ids().reserve(128);
  }

  /// @brief Constructor with a default configuration.
  encoder(const packet_handler_type& packet_handler)
    : encoder{packet_handler, configuration{}}
  {}

  /// @brief Give the encoder a new data.
  /// @param d The data to add.
  /// @attention Any use of the data @p d after this call will result in an undefined behavior,
  /// except for one case: calling data::reset() will put @p d in a usable state.
  void
  operator()(data&& d)
  {
    assert(d.used_bytes() != 0 && "please use data::used_bytes()");
    commit_impl(std::move(d));
  }

  /// @brief Give the encoder a new data.
  /// @param d The data to add.
  /// @attention Any use of the data @p d after this call will result in an undefined behavior,
  /// except for one case: calling data::reset() will put @p d in a usable state.
  void
  operator()(copy_data&& d)
  {
    commit_impl(std::move(d));
  }

  /// @brief Notify the encoder of a new incoming packet.
  /// @param p The incoming packet.
  /// @return false if the data could not have been decoded, true otherwise.
  /// @attention Any use of the packet @p d after this call will result in an undefined behavior,
  /// except for one case: calling packet::reset() will put @p p in a usable state.
  bool
  operator()(const packet& p)
  {
    return notify_impl(p.buffer());
  }

  /// @brief Notify the encoder of a new incoming packet.
  /// @param p The incoming packet.
  /// @return false if the data could not have been decoded, true otherwise.
  /// @attention Any use of the packet @p d after this call will result in an undefined behavior,
  /// except for one case: calling packet::reset() will put @p p in a usable state.
  bool
  operator()(const copy_packet& p)
  {
    return notify_impl(p.buffer());
  }

  /// @brief The number of packets which have not been acknowledged.
  std::size_t
  window_size()
  const noexcept
  {
    return sources_.size();
  }

  /// @brief Get the code rate.
  std::size_t
  rate()
  const noexcept
  {
    return conf_.rate;
  }

  /// @brief Set the code rate.
  std::size_t&
  rate()
  noexcept
  {
    return conf_.rate;
  }

  /// @brief Get the number of generated repairs.
  std::size_t
  nb_repairs()
  const noexcept
  {
    return nb_repairs_;
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

private:

  /// @brief Create a source from the given data and generate a repair if needed.
  /// @param d The data to add.
  /// @todo Handle possible allocation errors.
  template <typename Data>
  void
  commit_impl(Data&& d)
  {
    assert(d.buffer_impl().size() % 16 == 0 && "data buffer size not a multiple of 16");
    assert(d.used_bytes() <= d.buffer_impl().size() && "More bytes than the buffer can hold");

    if (sources_.size() == conf_.window)
    {
      sources_.pop_front();
    }

    // Create a new source in-place at the end of the list of sources, "stealing" the data
    // buffer from d.
    const auto& insertion
      = sources_.emplace(current_source_id_, std::move(d.buffer_impl()), d.used_bytes());

    // Ask packetizer to handle the bytes of the new source (will be routed to user's handler).
    packetizer_.write_source(insertion);

    /// @todo Should we generate a repair if window_size() == 1?
    if ((current_source_id_ + 1) % conf_.rate == 0)
    {
      send_repair();
    }

    current_source_id_ += 1;
  }

  /// @brief Notify the encoder that some data has been received.
  /// @return false if the data could not have been decoded, true otherwise.
  bool
  notify_impl(const char* data)
  {
    assert(data != nullptr);
    if (detail::get_packet_type(data) == detail::packet_type::ack)
    {
      const auto source_ids = packetizer_.read_ack(data).source_ids();
      sources_.erase(begin(source_ids), end(source_ids));
      return true;
    }
    return false;
  }

  /// @brief Launch the generation of a repair.
  void
  mk_repair()
  {
    // Set the identifier of the new repair (needed by the coder to generate coefficients).
    repair_.id() = current_repair_id_;

    // Create the repair packet from the list of sources.
    assert(sources_.size() > 0 && "Empty source list");
    encoder_(repair_, sources_.cbegin(), sources_.cend());

    current_repair_id_ += 1;
    nb_repairs_ += 1;
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
  std::size_t nb_repairs_;
};

/*------------------------------------------------------------------------------------------------*/

} // namespace ntc
