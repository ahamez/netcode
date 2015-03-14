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
  /// @return The newly added source.
  template <typename... Args>
  const_iterator
  emplace(Args&&... args)
  {
    return sources_.emplace(sources_.cend(), std::forward<Args>(args)...);
  }

  /// @brief Remove a source packet using its identifier.
  void
  erase(id_type id)
  noexcept
  {
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
