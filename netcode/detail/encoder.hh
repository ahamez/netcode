#pragma once

#include "netcode/detail/galois_field.hh"
#include "netcode/detail/repair.hh"
#include "netcode/detail/source.hh"
#include "netcode/detail/source_list.hh"

namespace ntc { namespace detail {

/*------------------------------------------------------------------------------------------------*/

/// @internal
/// @brief The component responsible for the encoding of detail::repair.
class encoder final
{
public:

  /// @brief Constructor.
  encoder(std::size_t galois_field_size);

  /// @brief Fill a @ref detail::repair from a set of detail::source.
  /// @param repair The repair to fill.
  /// @param sources The container of @ref detail::source to build the repair from.
  void
  operator()(repair& repair, source_list& sources);

private:

  /// @brief The implementation of a Galois field.
  detail::galois_field gf_;
};

/*------------------------------------------------------------------------------------------------*/

}} // namespace ntc::detail
