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
#include "netcode/packet.hh"
#include "netcode/systematic.hh"

namespace ntc {

/*------------------------------------------------------------------------------------------------*/

/// @brief The class to interact with on the sender side
/// @ingroup ntc_encoder
template <typename PacketHandler>
class encoder final
{
public:

  /// @brief The type of the handler that processes data ready to be sent on the network
  using packet_handler_type = PacketHandler;

public:

  /// @brief Can't copy-construct an encoder
  encoder(const encoder&) = delete;

  /// @brief Can't copy an encoder
  encoder& operator=(const encoder&) = delete;

  /// @brief Constructor
  template <typename PacketHandler_>
  encoder(std::uint8_t galois_field_size, PacketHandler_&& packet_handler)
    : m_galois_field_size{galois_field_size}
    , m_code_type{systematic::yes}
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
    // Let's reserve some memory for the repair, it will most likely avoid initial memory
    // allocations.
    m_repair.symbol().reserve(2048);
    // Same thing for the list of source identifiers.
    // Uncomment the following when the undefined behavior spotted by GCC 5.1 -fsanitize=undefined
    // is fixed. In the meantime, it's not a real problem,it will just cost a few initial
    // allocations before the source ids list grows to a suitable size.
    // m_repair.source_ids().reserve(128);
  }

  /// @brief Give the encoder a new data
  void
  operator()(const data& d)
  {
    operator()(data{d});
  }

  /// @brief Give the encoder a new data
  void
  operator()(data&& d)
  {
    assert(d.size() != 0 && "empty data");
    assert( m_galois_field_size != 16
            or (m_galois_field_size == 16 and d.size() % (16/8) == 0));
    assert( m_galois_field_size != 32
            or (m_galois_field_size == 32 and d.size() % (32/8) == 0));
    commit_impl(std::move(d));
  }

  /// @brief Notify the decoder of an incoming packet
  std::size_t
  operator()(const packet& p)
  {
    return operator()(packet{p});
  }

  /// @brief Notify the decoder of an incoming packet
  std::size_t
  operator()(packet&& p)
  {
    assert(p.size() != 0 && "empty packet");
    return notify_impl(std::move(p));
  }

  /// @brief The number of packets which have not been acknowledged
  std::size_t
  window()
  const noexcept
  {
    return m_sources.size();
  }

  /// @brief Get the number of received acks
  std::size_t
  nb_received_acks()
  const noexcept
  {
    return m_nb_acks;
  }

  /// @brief Get the number of sent repairs
  std::size_t
  nb_sent_repairs()
  const noexcept
  {
    return m_nb_sent_repairs;
  }

  /// @brief Get the number of sent sources
  std::size_t
  nb_sent_sources()
  const noexcept
  {
    return m_nb_sent_sources;
  }

  /// @brief Get the data handler
  const packet_handler_type&
  packet_handler()
  const noexcept
  {
    return m_packet_handler;
  }

  /// @brief Get the data handler
  packet_handler_type&
  packet_handler()
  noexcept
  {
    return m_packet_handler;
  }

  /// @brief Force the generation of a repair
  void
  generate_repair()
  {
    m_repair.reset();
    mk_repair();
    ++m_nb_sent_packets;
    m_packetizer.write_repair(m_repair);
  }

  /// @brief Get the Galois's field size
  std::uint8_t
  galois_field_size()
  const noexcept
  {
    return m_galois_field_size;
  }

  /// @brief Set the systematic/non-systematic mode of the code
  encoder&
  set_code_type(systematic c)
  noexcept
  {
    m_code_type = c;
    return *this;
  }

  /// @brief Get the systematic/non-systematic mode of the code
  systematic
  code_type()
  const noexcept
  {
    return m_code_type;
  }

  /// @brief Set how many sources are sent before a repair is generated
  /// @pre @p rate > 0
  encoder&
  set_rate(std::size_t rate)
  noexcept
  {
    assert(rate > 0);
    m_rate = rate;
    return *this;
  }

  /// @brief Get how many sources are sent before a repair is generated
  std::size_t
  rate()
  const noexcept
  {
    return m_rate;
  }

  /// @brief Set the maximal permitted size of the encoder's window
  /// @pre @p sz > 0
  encoder&
  set_window_size(std::size_t sz)
  noexcept
  {
    assert(sz > 0);
    m_window_size = sz;
    return *this;
  }

  /// @brief Get the maximal permitted size of the encoder's window
  std::size_t
  window_size()
  const noexcept
  {
    return m_window_size;
  }

  /// @brief Set the adaptive mode of the code
  encoder&
  set_adaptive(bool adaptive)
  noexcept
  {
    m_adaptive = adaptive;
    return *this;
  }

  /// @brief Get the adaptative mode of the code
  bool
  adaptive()
  const noexcept
  {
    return m_adaptive;
  }

private:

  /// @brief Create a source from the given data and generate a repair if needed
  /// @param d The data to add
  void
  commit_impl(data&& d)
  {
    if (m_sources.size() == m_window_size)
    {
      m_sources.pop_front();
    }

    // Create a new source in-place at the end of the list of sources.
    const auto& insertion = m_sources.emplace(m_current_source_id, std::move(d));

    if (m_code_type == systematic::yes)
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

  /// @brief Notify the encoder that some packet has been received (should be an ack)
  /// @return The number of bytes that have been read (0 if the packet was not decoded)
  /// @throw packet_type_error
  std::size_t
  notify_impl(packet&& p)
  {
    if (detail::get_packet_type(p) != detail::packet_type::ack)
    {
      throw packet_type_error{};
    }
    else
    {
      ++m_nb_acks;
      const auto res = m_packetizer.read_ack(std::move(p));
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
  }

  /// @brief Launch the generation of a repair
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

  /// @brief Compute the code rate needed for a given loss rate
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

  /// @brief The Galois field size
  const std::uint8_t m_galois_field_size;

  /// @brief Tell if the code is systematic or not
  systematic m_code_type;

  /// @brief How many sources to send before a repair is generated
  std::size_t m_rate;

  /// @brief The maximal number of sources to keep on the encoder side before discarding them
  std::size_t m_window_size;

  /// @brief Tell if the code is adaptive
  bool m_adaptive;

  /// @brief The counter for source packets identifiers
  std::uint32_t m_current_source_id;

  /// @brief The counter for repair packets identifiers
  std::uint32_t m_current_repair_id;

  /// @brief The set of souces which have not yet been acknowledged
  detail::source_list m_sources;

  /// @brief Re-use the same memory to prepare a repair packet
  detail::encoder_repair m_repair;

  /// @brief The user's handler
  packet_handler_type m_packet_handler;

  /// @brief The component that handles the coding process
  detail::encoder m_encoder;

  /// @brief How to read and write packets
  detail::packetizer<packet_handler_type> m_packetizer;

  /// @brief The number of generated repairs
  std::size_t m_nb_sent_repairs;

  /// @brief The number of received ack
  std::size_t m_nb_acks;

  /// @brief The number of sent sources
  std::size_t m_nb_sent_sources;

  /// @brief The number of sent packets since last ack
  std::uint16_t m_nb_sent_packets;
};

/*------------------------------------------------------------------------------------------------*/

} // namespace ntc
