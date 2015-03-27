#pragma once

#include <algorithm>  // all_of, is_sorted
#include <cassert>
#include <unordered_map>

#include "netcode/detail/galois_field.hh"
#include "netcode/detail/repair.hh"
#include "netcode/detail/source.hh"

namespace ntc { namespace detail {

/*------------------------------------------------------------------------------------------------*/

/// @internal
class decoder final
{
public:

  decoder(unsigned int galois_field_size, std::function<void(const source&)> h)
    : gf_{galois_field_size}
    , callback_(h)
    , sources_{}
    , repairs_{}
    , missing_sources_{}
  {}

  /// @todo Make sure we handle duplicate sources.
  void
  operator()(source&& src)
  {
    callback_(src);

    // First, look for all repairs that contain this incoming source in order to remove it
    // from these repairs.
    const auto search_range = missing_sources_.equal_range(src.id());
    for (auto cit = search_range.first; cit != search_range.second; ++cit)
    {
      // A reference to the current repair.
      auto& r = *cit->second;

      /// @todo remove src from r.

      // Remove src id of the list of the current repair source identifiers.
      const auto id_search = std::find(begin(r.source_ids()), end(r.source_ids()), src.id());
      assert(id_search != end(r.source_ids()) && "Source id not in current repair");
      r.source_ids().erase(id_search);

      if (r.source_ids().size() == 1)
      {
        /// @todo handle repair with only one source.

        // Source reconstructed, we can now safely erase the current repair as it is no longer
        // useful.
        repairs_.erase(r.id());
      }

      /// @todo Check that it's possible to combine several repairs in order to reconstruct
      /// missing sources.
    }

    // Now, there are no more repairs that reference the current source, thus we now update the
    // mapping src -> repairs.
    missing_sources_.erase(src.id());

    // Finally, insert this new source in the set of known sources.
    const auto src_id = src.id(); // to force evaluation order in the following call.
    sources_.emplace(src_id, std::move(src));
  }

  void
  operator()(repair&& r)
  {
    /// @todo Call callback_() when a source has been decoded.

    // By construction, the list of source identifiers should be sorted.
    assert(std::is_sorted(begin(r.source_ids()), end(r.source_ids())));

    // Remove sources with an id smaller than the smallest the current repair encodes.
    // Maybe not the smartest way to do it, we have to iterate _all_ sources. Maybe we should use
    // a different container (Boost.MultiIndex, std::set, etc.).
    const auto smallest_id = r.source_ids().front();
    for (auto cit = begin(sources_), cend = end(sources_); cit != cend;)
    {
      if (cit->first < smallest_id)
      {
        const auto to_erase = cit;
        ++cit;
        missing_sources_.erase(to_erase->first);
        sources_.erase(to_erase);
      }
      else
      {
        ++cit;
      }
    }

    /// Check if r is useless. Indeed, if all sources it references were correctly received, then
    /// it's useless to remove them from this repair, which is a costly operation.
    const auto useless = std::all_of( begin(r.source_ids()), end(r.source_ids())
                                    , [this](std::uint32_t src_id)
                                      {
                                        return sources_.count(src_id);
                                      });
    if (useless)
    {
      // Drop repair.
      return;
    }

    // Add this repair to the set of known repairs.
    const auto r_id = r.id(); // to force evaluation order in the following call.
    const auto insertion = repairs_.emplace(r_id, std::move(r));
    assert(insertion.second && "Repair with the same id already processed");

    // Don't use r beyond this point (as it was moved into repairs_), instead use r_ptr.
    auto* r_ptr = &insertion.first->second;

    // Reverse loop as vector::erase() invalidates iterators past the one being erased.
    for ( auto id_rcit = r_ptr->source_ids().rbegin(), end = r_ptr->source_ids().rend()
        ; id_rcit != end; ++id_rcit)
    {
      if (sources_.count(*id_rcit))
      {
        /// @todo remove src from r.

        // Get the 'normal' iterator corresponding to the current reverse iterator.
        // http://stackoverflow.com/a/1830240/21584
        const auto to_erase = std::next(id_rcit).base();
        r_ptr->source_ids().erase(to_erase);
      }
      else
      {
        // Link this repair with the sources it references.
        missing_sources_.emplace(*id_rcit, r_ptr);
      }
    }
    assert(not r_ptr->source_ids().empty());

    if (r_ptr->source_ids().size() == 1)
    {
      /// @todo handle repair with only one source.

      repairs_.erase(r_ptr->id());
      return;
    }

    /// @todo Check that it's possible to combine several repairs in order to reconstruct
    /// missing sources.
  }

private:

  /// @brief The callback to call when a source has been decoded.
  std::function<void(const source&)> callback_;

  /// @brief The set of received sources.
  std::unordered_map<std::uint32_t, source> sources_;

  /// @brief The set of received repairs.
  std::unordered_map<std::uint32_t, repair> repairs_;

  /// @brief All sources that have not been yet received, but which are referenced by a repair.
  std::unordered_multimap<std::uint32_t, repair*> missing_sources_;
};

/*------------------------------------------------------------------------------------------------*/

}} // namespace ntc::detail
