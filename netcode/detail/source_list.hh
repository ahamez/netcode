#pragma once

#include <list>

#include "netcode/detail/source.hh"
#include "netcode/detail/source_id_list.hh"

namespace ntc { namespace detail {

/*------------------------------------------------------------------------------------------------*/

/// @internal
/// @brief Hold a list of @ref encoder_source.
class source_list final
{
public:

  /// @brief An iterator on sources.
  using const_iterator = std::list<encoder_source>::const_iterator;

public:

  /// @brief Add a source packet in-place.
  /// @return A reference to the added source.
  const encoder_source&
  emplace(std::uint32_t id, byte_buffer&& symbol)
  {
    m_sources.emplace_back(id, std::move(symbol));
    return m_sources.back();
  }

  /// @brief Remove source packets from a list of identifiers.
  void
  erase(source_id_list::const_iterator id_cit, source_id_list::const_iterator id_end)
  noexcept
  {
    // m_sources is sorted by insertion (and thus by identifier).
    auto source_it = m_sources.begin();
    const auto source_end = m_sources.end();
    while (source_it != source_end and id_cit != id_end)
    {
      if (source_it->id() == *id_cit)
      {
        // We found an identifier to erase.
        source_it = m_sources.erase(source_it);
        ++id_cit;
      }
      else if (source_it->id() > *id_cit)
      {
        // The current source has an identifier greater than the current id to erase.
        // This means that this id was already removed in a previous call to erase().
        // We can safely ignore it.
        ++id_cit;
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
    return m_sources.size();
  }

  /// @brief Get an iterator to the first source.
  const_iterator
  cbegin()
  const noexcept
  {
    return m_sources.cbegin();
  }

  /// @brief Get an iterator to the end of sources.
  const_iterator
  cend()
  const noexcept
  {
    return m_sources.cend();
  }

  /// @brief Drop the first source.
  void
  pop_front()
  noexcept
  {
    m_sources.pop_front();
  }

private:

  /// @brief The real container of source packets.
  std::list<encoder_source> m_sources;
};

/*------------------------------------------------------------------------------------------------*/

}} // namespace ntc::detail
