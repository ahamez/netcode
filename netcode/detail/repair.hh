#pragma once

#include "netcode/detail/buffer.hh"
#include "netcode/detail/source_id_list.hh"
#include "netcode/packet.hh"

namespace ntc { namespace detail {

/*------------------------------------------------------------------------------------------------*/

/// @internal
/// @brief An encoder repair packet.
class encoder_repair final
{
public:

  /// @brief Can't copy-construct a repair.
  encoder_repair(const encoder_repair&) = delete;

  /// @brief Can't copy a repair.
  encoder_repair& operator=(const encoder_repair&) = delete;

  /// @brief Can move-construct a repair.
  encoder_repair(encoder_repair&&) = default;

  /// @brief Can move a repair.
  encoder_repair& operator=(encoder_repair&&) = default;

  /// @brief Construct a default repair, with a given identifier.
  explicit encoder_repair(std::uint32_t id)
    : m_id{id}
    , m_sources_ids{}
    , m_encoded_size{}
    , m_buffer{}
  {}

  /// @brief Construct with an existing list of source identifiers and a symbol.
  /// @note For tests
  encoder_repair( std::uint32_t id, std::uint16_t encoded_size, source_id_list&& ids
                , detail::zero_byte_buffer&& buffer)
    : m_id{id}
    , m_sources_ids{std::move(ids)}
    , m_encoded_size{encoded_size}
    , m_buffer{std::move(buffer)}
  {}

  /// @brief This repair's identifier.
  std::uint32_t
  id()
  const noexcept
  {
    return m_id;
  }

  /// @brief This repair's identifier (mutable).
  std::uint32_t&
  id()
  noexcept
  {
    return m_id;
  }

  /// @brief This repair's list of source identifiers.
  const source_id_list&
  source_ids()
  const noexcept
  {
    return m_sources_ids;
  }

  /// @brief This repair's list of source identifiers (mutable).
  source_id_list&
  source_ids()
  noexcept
  {
    return m_sources_ids;
  }

  /// @brief This repair's symbol.
  const detail::zero_byte_buffer&
  symbol()
  const noexcept
  {
    return m_buffer;
  }

  /// @brief This repair's symbol (mutable).
  detail::zero_byte_buffer&
  symbol()
  noexcept
  {
    return m_buffer;
  }

  /// @brief Reset this repair.
  ///
  /// List of source identifiers and symbol are resized to 0.
  void
  reset()
  noexcept
  {
    m_sources_ids.clear();
    m_buffer.clear();
  }

  /// @brief Get the encoded sizes of all sources this repair contains.
  std::uint16_t
  encoded_size()
  const noexcept
  {
    return m_encoded_size;
  }

  /// @brief Get the encoded sizes of all sources this repair contains.
  std::uint16_t&
  encoded_size()
  noexcept
  {
    return m_encoded_size;
  }

private:

  /// @brief This repair's unique identifier.
  std::uint32_t m_id;

  /// @brief The list of source identifiers.
  source_id_list m_sources_ids;

  /// @brief The encoded sizes of all sources this repair contains.
  std::uint16_t m_encoded_size;

  /// @brief This repair's symbol.
  detail::zero_byte_buffer m_buffer;
};

/*------------------------------------------------------------------------------------------------*/

/// @internal
/// @brief A decoder repair packet.
///
/// To avoid copies, a repair on the decoder side is constructed using directly the packet received
/// from the network.
class decoder_repair final
{
public:

  /// @brief Can't copy-construct a repair.
  decoder_repair(const decoder_repair&) = delete;

  /// @brief Can't copy a repair.
  decoder_repair& operator=(const decoder_repair&) = delete;

  /// @brief Can move-construct a repair.
  decoder_repair(decoder_repair&&) = default;

  /// @brief Can move a repair.
  decoder_repair& operator=(decoder_repair&&) = default;

  /// @brief Construct with an existing list of source identifiers and a symbol.
  decoder_repair( std::uint32_t id, std::uint16_t encoded_size, source_id_list&& ids
                , packet&& p, std::size_t symbol_size)
    : m_id{id}
    , m_sources_ids{std::move(ids)}
    , m_encoded_size{encoded_size}
    , m_symbol_buffer{std::move(p)}
    , m_symbol_size{static_cast<std::uint16_t>(symbol_size)}
  {}

  /// @brief This repair's identifier.
  std::uint32_t
  id()
  const noexcept
  {
    return m_id;
  }

  /// @brief This repair's identifier (mutable).
  std::uint32_t&
  id()
  noexcept
  {
    return m_id;
  }

  /// @brief This repair's list of source identifiers.
  const source_id_list&
  source_ids()
  const noexcept
  {
    return m_sources_ids;
  }

  /// @brief This repair's list of source identifiers (mutable).
  source_id_list&
  source_ids()
  noexcept
  {
    return m_sources_ids;
  }

  /// @brief This repair's symbol.
  const char*
  symbol()
  const noexcept
  {
    return m_symbol_buffer.symbol();
  }

  /// @brief This repair's symbol (mutable).
  char*
  symbol()
  noexcept
  {
    return m_symbol_buffer.symbol();
  }

  /// @brief Get the encoded sizes of all sources this repair contains.
  std::uint16_t
  encoded_size()
  const noexcept
  {
    return m_encoded_size;
  }

  /// @brief Get the encoded sizes of all sources this repair contains.
  std::uint16_t&
  encoded_size()
  noexcept
  {
    return m_encoded_size;
  }

  /// @brief Get the number of bytes in the user's symbol
  std::uint16_t
  symbol_size()
  const noexcept
  {
    return m_symbol_size;
  }

private:

  /// @brief This repair's unique identifier.
  std::uint32_t m_id;

  /// @brief The list of source identifiers.
  source_id_list m_sources_ids;

  /// @brief The encoded sizes of all sources this repair contains.
  std::uint16_t m_encoded_size;

  /// @brief This repair's symbol.
  packet m_symbol_buffer;

  /// @brief This repair's symbol size
  std::uint16_t m_symbol_size;
};

/*------------------------------------------------------------------------------------------------*/

}} // namespace ntc::detail
