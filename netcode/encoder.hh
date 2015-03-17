#pragma once

#include <memory> // unique_ptr

#include "netcode/detail/handler.hh"
#include "netcode/detail/make_protocol.hh"
#include "netcode/detail/packet_type.hh"
#include "netcode/detail/protocol/simple.hh"
#include "netcode/detail/repair.hh"
#include "netcode/detail/serializer.hh"
#include "netcode/detail/source.hh"
#include "netcode/detail/source_list.hh"
#include "netcode/code.hh"
#include "netcode/code_type.hh"
#include "netcode/packet.hh"
#include "netcode/protocol.hh"
#include "netcode/symbol.hh"

namespace ntc {

/*------------------------------------------------------------------------------------------------*/

/// @brief The class to interact with on the sender side.
class encoder final
{
public:

  /// @brief Can't copy-construct an encoder.
  encoder(const encoder&) = delete;

  /// @brief Can't copy an encoder.
  encoder& operator=(const encoder&) = delete;

  /// @brief Constructor
  template <typename Handler>
  encoder(Handler&& h, code&& coder, unsigned int code_rate, code_type type, protocol prot)
    : coder_{std::move(coder)}
    , rate_{code_rate == 0 ? 1 : code_rate}
    , type_{type}
    , current_source_id_{0}
    , current_repair_id_{0}
    , sources_{}
    , repair_{current_repair_id_}
    , handler_{new detail::handler_derived<Handler>(std::forward<Handler>(h))}
    , serializer_{mk_protocol(prot, *handler_)}
    , nb_repairs_{0ul}
  {
    // Let's reserve some memory for the repair, it will most likely avoid memory re-allocations.
    repair_.buffer().reserve(1024);
    // Same thing for the list of source identifiers.
    repair_.source_ids().reserve(128);
  }

  /// @brief Constructor for a systematic encoder using the simple protocol.
  template <typename Handler>
  encoder(Handler&& h, unsigned int code_rate)
    : encoder{ std::forward<Handler>(h)
             , code{8}
             , code_rate
             , code_type::systematic
             , protocol::simple}
  {}

  /// @brief Give the encoder a new symbol.
  /// @param sym The symbol to add.
  /// @attention Any use of the symbol @p sym after this call will result in an undefined behavior.
  void
  commit(symbol&& sym)
  {
    assert(sym.user_size_ != 0 && "user_size hasn't been set, please invoke symbol::user_size()");
    commit_impl(std::move(sym));
  }

  /// @brief Give the encoder a new symbol.
  /// @param sym The automatic symbol to add.
  /// @attention Any use of the symbol @p sym after this call will result in an undefined behavior.
  void
  commit(auto_symbol&& sym)
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
  commit(copy_symbol&& sym)
  {
    commit_impl(std::move(sym));
  }

  /// @brief Notify the encoder of a new incoming packet.
  /// @param p The incoming packet.
  /// @return false if the data could not have been decoded, true otherwise.
  bool
  notify(const packet& p)
  {
    return notify_impl(p.buffer());
  }

  /// @brief Notify the encoder of a new incoming packet.
  /// @param p The incoming packet.
  /// @return false if the data could not have been decoded, true otherwise.
  bool
  notify(const auto_packet& p)
  {
    return notify_impl(p.buffer_.data());
  }

  /// @brief Notify the encoder of a new incoming packet.
  /// @param p The incoming packet.
  /// @return false if the data could not have been decoded, true otherwise.
  bool
  notify(const copy_packet& p)
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
  unsigned int
  rate()
  const noexcept
  {
    return rate_;
  }

  /// @brief Set the code rate.
  unsigned int&
  rate()
  noexcept
  {
    return rate_;
  }

  /// @brief Get the number of generated repairs.
  std::size_t
  nb_repairs()
  const noexcept
  {
    return nb_repairs_;
  }

private:

  /// @brief Create a source from the given symbol and generate a repair if needed.
  /// @param sym The symbol to add.
  /// @todo Handle possible allocation errors.
  template <typename Symbol>
  void
  commit_impl(Symbol&& sym)
  {
    assert(sym.user_size_ <= sym.buffer_.size() && "More bytes are used than the buffer can hold.");

    // Create a new source in-place at the end of the list of sources, "stealing" the symbol
    // buffer from sym.
    const auto& insertion
      = sources_.emplace(current_source_id_, std::move(sym.buffer_), sym.user_size_);

    // Ask user to handle the bytes of the new source.
    serializer_->write_source(insertion);

    /// @todo Should we generate a repair if window_size() == 1?
    if ((current_source_id_ + 1) % rate_ == 0)
    {
      mk_repair();
      // Ask user to handle the bytes of the new repair.
      serializer_->write_repair(repair_);
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
      const auto source_ids = serializer_->read_ack(data).source_ids();
      sources_.erase(begin(source_ids), end(source_ids));
      return true;
    }
    return false;
  }

  /// @brief Launch the generation of a repair.
  void
  mk_repair()
  {
    // Only reset the 'virtual' size of the buffer, the reserved memory is kept, so
    // no memory re-allocation occurs.
    repair_.reset();

    // Set the identifier of the new repair (needed by the coder to generate coefficients).
    repair_.id() = current_repair_id_;

    // Create the repair packet from the list of sources.
    assert(sources_.size() > 0 && "Empty source list");
    coder_.encode(repair_, sources_.cbegin(), sources_.cend());

    current_repair_id_ += 1;
    nb_repairs_ += 1;
  }

private:

  /// @brief The component that handles the coding process.
  code coder_;

  /// @brief The number of source packets to send before sending a repair packet.
  unsigned int rate_;

  /// @brief Is the encoder systematic?
  code_type type_;

  /// @brief The counter for source packets identifiers.
  std::uint32_t current_source_id_;

  /// @brief The counter for repair packets identifiers.
  std::uint32_t current_repair_id_;

  /// @brief The set of souces which have not yet been acknowledged.
  detail::source_list sources_;

  /// @brief Re-use the same memory to prepare a repair packet.
  detail::repair repair_;

  /// @brief The user's handler for various callbacks.
  std::unique_ptr<detail::handler_base> handler_;

  /// @brief How to serialize packets.
  std::unique_ptr<detail::serializer> serializer_;

  /// @brief The number of generated repairs.
  std::size_t nb_repairs_;
};

/*------------------------------------------------------------------------------------------------*/

} // namespace ntc
