#pragma once

#include "netcode/detail/buffer.hh"
#include "netcode/detail/source_id_list.hh"

namespace ntc { namespace detail {

/*------------------------------------------------------------------------------------------------*/

/// @internal
/// @brief A repair packet.
class repair final
{
public:

  /// @brief Can't copy-construct a repair.
  repair(const repair&) = delete;

  /// @brief Can't copy a repair.
  repair& operator=(const repair&) = delete;

  /// @brief Can move-construct a repair.
  repair(repair&&) = default;

  /// @brief Can move a repair.
  repair& operator=(repair&&) = default;

  /// @brief Construct with an existing list of source identifiers and a symbol.
  repair( std::uint32_t id, std::uint16_t encoded_size, source_id_list&& ids
        , detail::zero_byte_buffer&& buffer)
    : m_id{id}
    , m_sources_ids{std::move(ids)}
    , m_encoded_size{encoded_size}
    , m_buffer{std::move(buffer)}
  {}

  /// @brief Construct a default repair, with a given identifier.
  explicit repair(std::uint32_t id)
    : m_id{id}
    , m_sources_ids{}
    , m_encoded_size{}
    , m_buffer{}
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
  buffer()
  const noexcept
  {
    return m_buffer;
  }

  /// @brief This repair's symbol (mutable).
  detail::zero_byte_buffer&
  buffer()
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

}} // namespace ntc::detail
