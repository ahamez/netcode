#pragma once

#include <memory> // unique_ptr

#include "netcode/detail/handler.hh"
#include "netcode/detail/packet_type.hh"
#include "netcode/detail/protocol/simple.hh"
#include "netcode/detail/repair.hh"
#include "netcode/detail/serializer.hh"
#include "netcode/detail/source.hh"
#include "netcode/detail/source_list.hh"
#include "netcode/coding.hh"
#include "netcode/symbol.hh"
#include "netcode/types.hh"

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
  encoder(Handler&& h, const coding& c, unsigned int code_rate, code_type type)
    : coding_{c}
    , rate_{code_rate == 0 ? 1 : code_rate}
    , type_{type}
    , current_source_id_{0}
    , current_repair_id_{0}
    , sources_{}
    , repair_{current_repair_id_}
    , handler_{new detail::handler_derived<Handler>(std::forward<Handler>(h))}
    , serializer_{new detail::protocol::simple{*handler_}}
    , nb_repairs_{0ul}
  {
    // Let's reserve some memory for the repair, it will most likely avoid memory re-allocations.
    repair_.buffer().reserve(512);
    // The same for the list of source identifiers.
    repair_.source_ids().reserve(64);
  }

  /// @brief Constructor
  template <typename Handler>
  encoder(Handler&& h, const coding& c, unsigned int code_rate)
    : encoder{std::forward<Handler>(h), c, code_rate, code_type::systematic}
  {}

  /// @brief Notify the encoder that some data has been received.
  /// @return false if the data could not have been decoded, true otherwise.
  bool
  notify(const char* data)
  {
    if (detail::get_packet_type(data) == detail::packet_type::ack)
    {
      const auto source_ids = serializer_->read_ack(data).source_ids();
      sources_.erase(begin(source_ids), end(source_ids));
      return true;
    }
    return false;
  }

  /// @brief Give the encoder a new symbol.
  /// @param sym The symbol to add.
  ///
  /// The symbol @p sym won't be usable after this call.
  void
  commit(symbol&& sym)
  {
    commit_impl(std::move(sym));
  }

  /// @brief Give the encoder a new symbol.
  /// @param sym The automatic symbol to add.
  ///
  /// The symbol @p sym won't be usable after this call.
  void
  commit(auto_symbol&& sym)
  {
    commit_impl(std::move(sym));
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
  /// @todo Should a new smaller rate be treated specifically?.
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
  void
  commit_impl(detail::symbol_base&& sym)
  {
    // Create a new source in-place at the end of the list of sources, "stealing" the symbol
    // buffer from sym.
    const auto insertion = sources_.emplace(current_source_id_, std::move(sym.buffer()));

    // Ask user to handle the bytes of the new source.
    serializer_->write_source(*insertion);

    // Should we generate a repair?
    if ((current_source_id_ + 1) % rate_ == 0)
    {
      make_repair();
    }

    current_source_id_ += 1;
  }

  /// @brief Order the generation of a repair.
  void
  make_repair()
  {
    // Only reset the 'virtual' size of the buffer, the reserved memory is kept.
    repair_.reset();

    // Create the repair packet from the list of sources.
    coding_(repair_, sources_.cbegin(), sources_.cend());

    // Set the identifier of the new repair.
    repair_.id() = current_repair_id_;

    // Ask user to handle the bytes of the new repair.
    serializer_->write_repair(repair_);

    current_repair_id_ += 1;
    nb_repairs_ += 1;
  }

  /// @brief The component that handles the coding process.
  coding coding_;

  /// @brief The number of source packets to send before sending a repair packet.
  unsigned int rate_;

  /// @brief Is the encoder systematic?
  code_type type_;

  /// @brief The counter for source packets identifiers.
  id_type current_source_id_;

  /// @brief The counter for repair packets identifiers.
  id_type current_repair_id_;

  /// @brief Re-use the same memory to prepare a repair packet.
  detail::repair repair_;

  /// @brief The set of souces which have not yet been acknowledged.
  detail::source_list sources_;

  /// @brief The user's handler for various callbacks.
  std::unique_ptr<detail::handler_base> handler_;

  /// @brief How to serialize packets.
  std::unique_ptr<detail::serializer> serializer_;

  /// @brief The number of generated repairs.
  std::size_t nb_repairs_;
};

/*------------------------------------------------------------------------------------------------*/

} // namespace ntc
