#pragma once

#include <list>

#include "netcode/detail/source.hh"
#include "netcode/detail/source_id_list.hh"

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
    sources_.emplace_back(std::forward<Args>(args)...);
    return sources_.back();
  }

  /// @brief Remove source packets from a list of identifiers.
  void
  erase(source_id_list::const_iterator id_cit, source_id_list::const_iterator id_end)
  noexcept
  {
    // sources_ is sorted by insertion (and thus by identifier).
    auto source_it = sources_.begin();
    const auto source_end = sources_.end();
    while (source_it != source_end and id_cit != id_end)
    {
      if (source_it->id() == *id_cit)
      {
        auto to_erase = source_it;
        ++source_it;
        ++id_cit;
        sources_.erase(to_erase);
      }
      else
      {
        ++source_it;
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

  /// @brief Drop the first source.
  void
  pop_front()
  noexcept
  {
    sources_.pop_front();
  }

private:

  /// @brief The real container of source packets.
  std::list<detail::source> sources_;
};

/*------------------------------------------------------------------------------------------------*/

}} // namespace ntc::detail
