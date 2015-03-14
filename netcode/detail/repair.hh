#pragma once

#include <vector>

#include "netcode/detail/symbol_buffer.hh"
#include "netcode/types.hh"

namespace ntc { namespace detail {

/*------------------------------------------------------------------------------------------------*/

/// @internal
/// @brief A repair packet.
class repair final
{
public:

  /// @brief Construct with an existing list of source identifiers and a symbol.
  repair(id_type id, std::vector<id_type>&& ids, detail::symbol_buffer&& buffer)
    : id_{id}
    , sources_ids_{std::move(ids)}
    , buffer_{std::move(buffer)}
  {}

  /// @brief Construct a default repair, with a given identifier.
  repair(id_type id)
    : id_{id}
    , sources_ids_{}
    , buffer_{}
  {}

  /// @brief This repair's identifier.
  id_type
  id()
  const noexcept
  {
    return id_;
  }

  /// @brief This repair's identifier (mutable).
  id_type&
  id()
  noexcept
  {
    return id_;
  }

  /// @brief This repair's list of source identifiers.
  const std::vector<id_type>&
  source_ids()
  const noexcept
  {
    return sources_ids_;
  }

  /// @brief This repair's list of source identifiers (mutable).
  std::vector<id_type>&
  source_ids()
  noexcept
  {
    return sources_ids_;
  }

  /// @brief This repair's symbol.
  const detail::symbol_buffer&
  buffer()
  const noexcept
  {
    return buffer_;
  }

  /// @brief This repair's symbol (mutable).
  detail::symbol_buffer&
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
  id_type id_;

  /// @brief The list of source identifiers.
  std::vector<id_type> sources_ids_;

  /// @brief This repair's symbol.
  detail::symbol_buffer buffer_;
};

/*------------------------------------------------------------------------------------------------*/

}} // namespace ntc::detail
