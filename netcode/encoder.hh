#pragma once

#include <memory>

#include "netcode/coding.hh"
#include "netcode/symbol.hh"
#include "netcode/types.hh"
#include "netcode/detail/handler.hh"
#include "netcode/detail/protocol/simple.hh"
#include "netcode/detail/repair.hh"
#include "netcode/detail/serializer.hh"
#include "netcode/detail/source.hh"
#include "netcode/detail/source_list.hh"

namespace ntc {

/*------------------------------------------------------------------------------------------------*/

/// @brief
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
    , handler_{new detail::handler_derived<Handler>{std::forward<Handler>(h)}}
    , serializer_{new detail::protocol::simple{*handler_}}
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
        case detail::packet_type::ack:
        {
          for (auto id : serializer_->read_ack(data).source_ids())
          {
            sources_.erase(id);
          }
          return true;
        }

        default: return false;
    }
  }

  /// @brief Give the encoder a new symbol.
  /// @param sym The symbol to add.
  ///
  /// @p sym won't be usable after this call.
  void
  commit_symbol(symbol_base&& sym)
  {
    // Create a new source in-place at the end of the list of sources, "stealing" the symbol
    // buffer from sym.
    sources_.emplace(current_source_id_, std::move(sym.symbol_buffer()));

    // Ask user to handle the bytes of the new source.
    serializer_->write_source(sources_.last());

    // Should we generate a repair?
    if ((current_source_id_ + 1) % rate_ == 0)
    {
      repair_.id() = current_repair_id_;

      /// @todo Create repair content.

      // Ask user to handle the bytes of the new repair.
      serializer_->write_repair(repair_);
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
};

/*------------------------------------------------------------------------------------------------*/

} // namespace ntc
