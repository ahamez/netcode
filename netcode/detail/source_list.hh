#pragma once

#include <algorithm>
#include <list>

#include "netcode/detail/source.hh"
#include "netcode/detail/types.hh"

namespace ntc { namespace detail {

/*------------------------------------------------------------------------------------------------*/

/// @internal
/// @brief Hold a list of @ref source.
/// @todo Compare with an std::unordered_set.
class source_list final
{
public:

  /// @brief An iterator on sources.
  using const_iterator = std::list<detail::source>::const_iterator;

  /// @brief Add a source packet in-place.
  /// @return The newly added source.
  template <typename... Args>
  const detail::source&
  emplace(Args&&... args)
  {
    return *sources_.emplace(sources_.cend(), std::forward<Args>(args)...);
  }

  /// @brief Remove source packets from a list of identifiers.
  void
  erase(source_id_list::const_iterator id_cit, source_id_list::const_iterator id_end)
  noexcept
  {
    // sources_ is sorted by insertion (and thus by identifier).
    auto source_cit = sources_.cbegin();
    const auto source_end = sources_.cend();
    while (source_cit != source_end and id_cit != id_end)
    {
      if (source_cit->id() == *id_cit)
      {
        const auto to_erase = source_cit;
        ++source_cit;
        ++id_cit;
        sources_.erase(to_erase);
      }
      else
      {
        ++source_cit;
      }
    }
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
