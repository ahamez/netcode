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

  /// @brief An iterator on sources.
  using const_iterator = std::list<detail::source>::const_iterator;

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

  /// @brief Get an iterator to the first source.
  const_iterator
  cbegin()
  const noexcept
  {
    return sources_.cbegin();
  }

  /// @brief Get an iterator to the end of sources.
  const_iterator
  cend()
  const noexcept
  {
    return sources_.cend();
  }

private:

  /// @brief The real container of source packets.
  std::list<detail::source> sources_;
};

/*------------------------------------------------------------------------------------------------*/

}} // namespace ntc::detail
