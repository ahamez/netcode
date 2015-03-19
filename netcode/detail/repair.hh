#pragma once

#include "netcode/detail/raw_buffer.hh"
#include "netcode/detail/types.hh"

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

  /// @brief Can move a source.
  repair& operator=(repair&&) = delete;

  /// @brief Construct with an existing list of source identifiers and a symbol.
  repair(std::uint32_t id, source_id_list&& ids, detail::zero_raw_buffer&& buffer)
    : id_{id}
    , sources_ids_{std::move(ids)}
    , buffer_{std::move(buffer)}
  {}

  /// @brief Construct a default repair, with a given identifier.
  repair(std::uint32_t id)
    : id_{id}
    , sources_ids_{}
    , buffer_{}
  {}

  /// @brief This repair's identifier.
  std::uint32_t
  id()
  const noexcept
  {
    return id_;
  }

  /// @brief This repair's identifier (mutable).
  std::uint32_t&
  id()
  noexcept
  {
    return id_;
  }

  /// @brief This repair's list of source identifiers.
  const source_id_list&
  source_ids()
  const noexcept
  {
    return sources_ids_;
  }

  /// @brief This repair's list of source identifiers (mutable).
  source_id_list&
  source_ids()
  noexcept
  {
    return sources_ids_;
  }

  /// @brief This repair's symbol.
  const detail::zero_raw_buffer&
  buffer()
  const noexcept
  {
    return buffer_;
  }

  /// @brief This repair's symbol (mutable).
  detail::zero_raw_buffer&
  buffer()
  noexcept
  {
    return buffer_;
  }

  /// @brief Reset this repair.
  ///
  /// List of source identifiers and symbol are resized to 0.
  void
  reset()
  noexcept
  {
    sources_ids_.resize(0);
    buffer_.resize(0);
  }

private:

  /// @brief This repair's unique identifier.
  std::uint32_t id_;

  /// @brief The list of source identifiers.
  source_id_list sources_ids_;

  /// @brief This repair's symbol.
  detail::zero_raw_buffer buffer_;
};

/*------------------------------------------------------------------------------------------------*/

}} // namespace ntc::detail
