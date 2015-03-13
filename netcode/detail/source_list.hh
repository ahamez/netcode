#pragma once

#include <list>

#include "netcode/detail/source.hh"

namespace ntc { namespace detail {

/*------------------------------------------------------------------------------------------------*/

/// @internal
/// @brief Hold a list of sources.
class source_list final
{
public:

  /// @brief Add a source packet in-place.
  template <typename... Args>
  void
  emplace(Args&&... args)
  {
    sources_.emplace_back(std::forward<Args>(args)...);
  }

  /// @brief Remove a source packet using its identifier.
  void
  erase(id_type id)
  noexcept
  {
  }

  /// @brief Get the most recently added source.
  const source&
  last()
  const noexcept
  {
    return sources_.back();
  }

  /// @brief The number of source packets.
  std::size_t
  size()
  const noexcept
  {
    return sources_.size();
  }

private:

  /// @brief The real container of source packets.
  std::list<detail::source> sources_;
};

/*------------------------------------------------------------------------------------------------*/

}} // namespace ntc::detail
