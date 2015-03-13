#pragma once

#include <list>
#include <memory>

#include "netcode/coding.hh"
#include "netcode/symbol.hh"
#include "netcode/types.hh"
#include "netcode/detail/handler.hh"
#include "netcode/detail/repair.hh"
#include "netcode/detail/source.hh"

namespace ntc {

/*------------------------------------------------------------------------------------------------*/

/// @brief
class encoder
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
    , handler_ptr_{new detail::handler_derived<Handler>{std::forward<Handler>(h)}}
    , on_ready_packet_{[this](std::size_t nb, const char* data){handler_ptr_->on_ready_packet(nb, data);}}
  {}

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
    switch (static_cast<detail::packet_type>(data[0]))
    {
        case detail::packet_type::ack    : clear_sources(); break;
        case detail::packet_type::repair : break;
        case detail::packet_type::source : break;
        default                          : break;
    }
    return {};
  }

  void
  commit_symbol(symbol_base&& sym)
  {
    sources_.emplace_back(current_source_id_, std::move(sym.symbol_buffer()));
    sources_.back().write(on_ready_packet_);

    // Should we generate a repair?
    if ((current_source_id_ + 1) % rate_ == 0)
    {
      repair_.id() = current_repair_id_;
      repair_.write(on_ready_packet_);
    }

    current_source_id_ += 1;
    current_repair_id_ += 1;
  }

  /// @brief The number of packets which have not been acknowledged.
  std::size_t
  window_size()
  const noexcept
  {
    return sources_.size();
  }

private:

  /// @brief Delete sources which have been acknowledged.
  void
  clear_sources()
  noexcept
  {
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

  /// @brief The set of souces which have not yet been acknowledged.
  std::list<detail::source> sources_;

  /// @brief Re-use the same memory to prepare a repair packet.
  detail::repair repair_;

  /// @brief The user's handler for various callbacks.
  std::unique_ptr<detail::handler_base> handler_ptr_;

  /// @brief Shortcut to the packet writer in the user's handler.
  std::function<detail::on_ready_packet_fn> on_ready_packet_;
};

/*------------------------------------------------------------------------------------------------*/

} // namespace ntc
