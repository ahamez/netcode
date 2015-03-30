#pragma once

#include <algorithm> // is_sorted

#include "netcode/detail/encoder.hh"
#include "netcode/detail/packet_type.hh"
#include "netcode/detail/packetizer.hh"
#include "netcode/detail/repair.hh"
#include "netcode/detail/source.hh"
#include "netcode/detail/source_list.hh"
#include "netcode/configuration.hh"
#include "netcode/code.hh"
#include "netcode/packet.hh"
#include "netcode/symbol.hh"

namespace ntc {

/*------------------------------------------------------------------------------------------------*/

/// @brief The class to interact with on the sender side.
template <typename DataHandler>
class encoder final
{
public:

  /// @brief The type of the handler that processes data ready to be sent on the network.
  using data_handler_type = DataHandler;

public:

  /// @brief Can't copy-construct an encoder.
  encoder(const encoder&) = delete;

  /// @brief Can't copy an encoder.
  encoder& operator=(const encoder&) = delete;

  /// @brief Constructor.
  encoder(const data_handler_type& data_handler, configuration conf)
    : conf_{conf}
    , current_source_id_{0}
    , current_repair_id_{0}
    , sources_{}
    , repair_{current_repair_id_}
    , data_handler_(data_handler)
    , encoder_{conf.galois_field_size}
    , packetizer_{data_handler_}
    , nb_repairs_{0ul}
  {
    assert(conf_.rate > 0);
    // Let's reserve some memory for the repair, it will most likely avoid memory re-allocations.
    repair_.buffer().reserve(2048);
    // Same thing for the list of source identifiers.
    repair_.source_ids().reserve(128);
  }

  /// @brief Constructor with a default configuration.
  encoder(const data_handler_type& data_handler)
    : encoder{data_handler, configuration{}}
  {}

  /// @brief Give the encoder a new symbol.
  /// @param sym The symbol to add.
  /// @attention Any use of the symbol @p sym after this call will result in an undefined behavior.
  void
  operator()(symbol&& sym)
  {
    assert(sym.user_size_ != 0 && "user_size hasn't been set, please invoke symbol::user_size()");
    commit_impl(std::move(sym));
  }

  /// @brief Give the encoder a new symbol.
  /// @param sym The automatic symbol to add.
  /// @attention Any use of the symbol @p sym after this call will result in an undefined behavior.
  void
  operator()(auto_symbol&& sym)
  {
    sym.user_size_ = sym.buffer_.size();
    if ((sym.buffer_.size() % 16) != 0)
    {
      // The automatic buffer has a size which is not a multiple of 16.
      sym.buffer_.resize(detail::make_multiple(sym.buffer_.size(), 16));
    }
    commit_impl(std::move(sym));
  }

  /// @brief Give the encoder a new symbol.
  /// @param sym The symbol to add.
  /// @attention Any use of the symbol @p sym after this call will result in an undefined behavior.
  void
  operator()(copy_symbol&& sym)
  {
    commit_impl(std::move(sym));
  }

  /// @brief Notify the encoder of a new incoming packet.
  /// @param p The incoming packet.
  /// @return false if the data could not have been decoded, true otherwise.
  bool
  operator()(const packet& p)
  {
    return notify_impl(p.buffer());
  }

  /// @brief Notify the encoder of a new incoming packet.
  /// @param p The incoming packet.
  /// @return false if the data could not have been decoded, true otherwise.
  bool
  operator()(const auto_packet& p)
  {
    return notify_impl(p.buffer_.data());
  }

  /// @brief Notify the encoder of a new incoming packet.
  /// @param p The incoming packet.
  /// @return false if the data could not have been decoded, true otherwise.
  bool
  operator()(const copy_packet& p)
  {
    return notify_impl(p.buffer_.data());
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

  /// @brief Force the sending of a repair.
  void
  send_repair()
  {
    repair_.reset();
    mk_repair();
    packetizer_.write_repair(repair_);
  }

private:

  /// @brief Create a source from the given symbol and generate a repair if needed.
  /// @param sym The symbol to add.
  /// @todo Handle possible allocation errors.
  template <typename Symbol>
  void
  commit_impl(Symbol&& sym)
  {
    assert(sym.buffer_.size() % 16 == 0 && "symbol buffer size not a multiple of 16");
    assert(sym.user_size_ <= sym.buffer_.size() && "More bytes are used than the buffer can hold");

    if (sources_.size() == conf_.window)
    {
      sources_.pop_front();
    }

    // Create a new source in-place at the end of the list of sources, "stealing" the symbol
    // buffer from sym.
    const auto& insertion
      = sources_.emplace(current_source_id_, std::move(sym.buffer_), sym.user_size_);

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
      // Identifiers should be sorted.
      assert(std::is_sorted(begin(source_ids), end(source_ids)));
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

    // By construction, the list of source identifiers should be sorted.
    assert(std::is_sorted(begin(repair_.source_ids()), end(repair_.source_ids())));

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
  data_handler_type data_handler_;

  /// @brief The component that handles the coding process.
  detail::encoder encoder_;

  /// @brief How to serialize packets.
  detail::packetizer<data_handler_type> packetizer_;

  /// @brief The number of generated repairs.
  std::size_t nb_repairs_;
};

/*------------------------------------------------------------------------------------------------*/

} // namespace ntc
