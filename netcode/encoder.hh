#pragma once

#include <algorithm>
#include <cassert>
#include <chrono>
#include <cmath>
#include <limits> // numeric_limits

#include "netcode/detail/encoder.hh"
#include "netcode/detail/packet_type.hh"
#include "netcode/detail/packetizer.hh"
#include "netcode/detail/repair.hh"
#include "netcode/detail/source.hh"
#include "netcode/detail/source_list.hh"
#include "netcode/data.hh"
#include "netcode/errors.hh"

namespace ntc {

/*------------------------------------------------------------------------------------------------*/

/// @brief Describe if a code is systematic or not.
/// @ingroup ntc_configuration
enum class code {systematic, non_systematic};

/*------------------------------------------------------------------------------------------------*/

/// @brief The class to interact with on the sender side.
/// @ingroup ntc_encoder
template <typename PacketHandler>
class encoder final
{
public:

  /// @brief The type of the handler that processes data ready to be sent on the network.
  using packet_handler_type = PacketHandler;

public:

  /// @brief Can't copy-construct an encoder.
  encoder(const encoder&) = delete;

  /// @brief Can't copy an encoder.
  encoder& operator=(const encoder&) = delete;

  /// @brief Constructor.
  template <typename PacketHandler_>
  encoder(std::uint8_t galois_field_size, PacketHandler_&& packet_handler)
    : m_galois_field_size{galois_field_size}
    , m_code_type{code::systematic}
    , m_rate{5}
    , m_window_size{std::numeric_limits<std::size_t>::max()}
    , m_adaptive{false}
    , m_current_source_id{0}
    , m_current_repair_id{0}
    , m_sources{}
    , m_repair{m_current_repair_id}
    , m_packet_handler(std::forward<PacketHandler_>(packet_handler))
    , m_encoder{m_galois_field_size}
    , m_packetizer{m_packet_handler}
    , m_nb_sent_repairs{0ul}
    , m_nb_acks{0ul}
    , m_nb_sent_sources{0ul}
    , m_nb_sent_packets{0}
  {
    // Let's reserve some memory for the repair, it will most likely avoid memory re-allocations.
    m_repair.buffer().reserve(2048);
    // Same thing for the list of source identifiers.
    m_repair.source_ids().reserve(128);
  }

  /// @brief Give the encoder a new data.
  /// @param d The data to add.
  /// @attention Any use of the data @p d after this call will result in an undefined behavior,
  /// except for one case: calling data::reset() will put back @p d in a usable state.
  /// @see data::reset
  void
  operator()(data&& d)
  {
    assert(d.used_bytes() != 0 && "please use data::used_bytes()");
    assert( m_galois_field_size != 16
            or (m_galois_field_size == 16 and d.used_bytes() % (16/8) == 0));
    assert( m_galois_field_size != 32
            or (m_galois_field_size == 32 and d.used_bytes() % (32/8) == 0));
    commit_impl(std::move(d));
  }

  /// @brief Notify the encoder of a new incoming packet.
  /// @param packet The incoming packet.
  /// @param max_len The maximum number of bytes to read from @p packet.
  /// @return The number of bytes that have been read.
  /// @throw overflow_error when the number of read bytes > @p max_len.
  /// @throw packet_type_error when the packet has an incorrect type.
  std::size_t
  operator()(const char* packet, std::size_t max_len)
  {
    return notify_impl(packet, max_len);
  }

  /// @brief Notify the encoder of a new incoming packet.
  /// @param packet The incoming packet stored in a vector.
  /// @return The number of bytes that have been read.
  /// @throw packet_type_error when the packet has an incorrect type.
  std::size_t
  operator()(const std::vector<char>& packet)
  {
    return operator()(packet.data(), packet.size());
  }

  /// @brief The number of packets which have not been acknowledged.
  std::size_t
  window()
  const noexcept
  {
    return m_sources.size();
  }

  /// @brief Get the number of received acks.
  std::size_t
  nb_received_acks()
  const noexcept
  {
    return m_nb_acks;
  }

  /// @brief Get the number of sent repairs.
  std::size_t
  nb_sent_repairs()
  const noexcept
  {
    return m_nb_sent_repairs;
  }

  /// @brief Get the number of sent sources.
  std::size_t
  nb_sent_sources()
  const noexcept
  {
    return m_nb_sent_sources;
  }

  /// @brief Get the data handler.
  const packet_handler_type&
  packet_handler()
  const noexcept
  {
    return m_packet_handler;
  }

  /// @brief Get the data handler.
  packet_handler_type&
  packet_handler()
  noexcept
  {
    return m_packet_handler;
  }

  /// @brief Force the generation of a repair.
  void
  generate_repair()
  {
    m_repair.reset();
    mk_repair();
    ++m_nb_sent_packets;
    m_packetizer.write_repair(m_repair);
  }

  /// @brief Get the Galois's field size.
  std::uint8_t
  galois_field_size()
  const noexcept
  {
    return m_galois_field_size;
  }

  /// @brief Set the systematic/non-systematic mode of the code.
  void
  set_code_type(code c)
  noexcept
  {
    m_code_type = c;
  }

  /// @brief Get the systematic/non-systematic mode of the code.
  code
  code_type()
  const noexcept
  {
    return m_code_type;
  }

  /// @brief Set how many sources are sent before a repair is generated.
  /// @pre @p rate > 0
  void
  set_rate(std::size_t rate)
  noexcept
  {
    assert(rate > 0);
    m_rate = rate;
  }

  /// @brief Get how many sources are sent before a repair is generated.
  std::size_t
  rate()
  const noexcept
  {
    return m_rate;
  }

  /// @brief Set the maximal permitted size of the encoder's window.
  /// @pre @p sz > 0
  void
  set_window_size(std::size_t sz)
  noexcept
  {
    assert(sz > 0);
    m_window_size = sz;
  }

  /// @brief Get the maximal permitted size of the encoder's window.
  std::size_t
  window_size()
  const noexcept
  {
    return m_window_size;
  }

  /// @brief Set the adaptive mode of the code.
  void
  set_adaptive(bool adaptive)
  noexcept
  {
    m_adaptive = adaptive;
  }

  /// @brief Get the adaptative mode of the code.
  bool
  adaptive()
  const noexcept
  {
    return m_adaptive;
  }

private:

  /// @brief Create a source from the given data and generate a repair if needed.
  /// @param d The data to add.
  void
  commit_impl(data&& d)
  {
    assert(d.used_bytes() <= d.buffer_impl().size() && "More used bytes than the buffer can hold");

    if (m_sources.size() == m_window_size)
    {
      m_sources.pop_front();
    }

    // Create a new source in-place at the end of the list of sources, "stealing" the data
    // buffer from d.
    const auto& insertion
      = m_sources.emplace(m_current_source_id, std::move(d.buffer_impl()), d.used_bytes());

    if (m_code_type == code::systematic)
    {
      ++m_nb_sent_sources;
      ++m_nb_sent_packets;
      // Ask packetizer to handle the bytes of the new source (will be routed to user's handler).
      m_packetizer.write_source(insertion);
    }
    else // non_systematic code
    {
      generate_repair();
    }

    /// @todo Should we generate a repair if window_size() == 1?
    if ((m_current_source_id + 1) % m_rate == 0)
    {
      generate_repair();
    }

    ++m_current_source_id;
  }

  /// @brief Notify the encoder that some data has been received.
  /// @return The number of bytes that have been read (0 if the packet was not decoded).
  /// @throw packet_type_error
  std::size_t
  notify_impl(const char* data, std::size_t max_len)
  {
    assert(data != nullptr);
    if (detail::get_packet_type(data) == detail::packet_type::ack)
    {
      ++m_nb_acks;
      const auto res = m_packetizer.read_ack(data, max_len);
      if (m_adaptive)
      {
        if (m_nb_sent_packets > 0)
        {
          const auto loss_rate
            = (m_nb_sent_packets - res.first.nb_packets()) / static_cast<double>(m_nb_sent_packets);
          m_rate = rate_for_loss(loss_rate);
        }
        else
        {
          m_rate = rate_for_loss(0.0);
        }
      }
      m_nb_sent_packets = 0;
      m_sources.erase(begin(res.first.source_ids()), end(res.first.source_ids()));
      return res.second;
    }
    else
    {
      throw packet_type_error{};
    }
  }

  /// @brief Launch the generation of a repair.
  void
  mk_repair()
  {
    // Set the identifier of the new repair (needed by the coder to generate coefficients).
    m_repair.id() = m_current_repair_id;

    // Create the repair packet from the list of sources.
    assert(m_sources.size() > 0 && "Empty source list");
    m_encoder(m_repair, m_sources);

    ++m_current_repair_id;
    ++m_nb_sent_repairs;
  }

  /// @brief Compute the code rate needed for a given loss rate.
  static
  std::size_t
  rate_for_loss(double loss)
  noexcept
  {
    return loss < 0.01
         ? 50
         : static_cast<std::size_t>(ceil((1/loss)/2));
  }

private:

  /// @brief The Galois field size.
  const std::uint8_t m_galois_field_size;

  /// @brief Tell if the code is systematic or not.
  code m_code_type;

  /// @brief How many sources to send before a repair is generated.
  std::size_t m_rate;

  /// @brief The maximal number of sources to keep on the encoder side before discarding them.
  std::size_t m_window_size;

  /// @brief Tell if the code is adaptive.
  bool m_adaptive;

  /// @brief The counter for source packets identifiers.
  std::uint32_t m_current_source_id;

  /// @brief The counter for repair packets identifiers.
  std::uint32_t m_current_repair_id;

  /// @brief The set of souces which have not yet been acknowledged.
  detail::source_list m_sources;

  /// @brief Re-use the same memory to prepare a repair packet.
  detail::repair m_repair;

  /// @brief The user's handler.
  packet_handler_type m_packet_handler;

  /// @brief The component that handles the coding process.
  detail::encoder m_encoder;

  /// @brief How to read and write packets.
  detail::packetizer<packet_handler_type> m_packetizer;

  /// @brief The number of generated repairs.
  std::size_t m_nb_sent_repairs;

  /// @brief The number of received ack.
  std::size_t m_nb_acks;

  /// @brief The number of sent sources.
  std::size_t m_nb_sent_sources;

  /// @brief The number of sent packets since last ack.
  std::uint16_t m_nb_sent_packets;
};

/*------------------------------------------------------------------------------------------------*/

} // namespace ntc
