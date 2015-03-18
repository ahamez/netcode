#pragma once

#include <algorithm>  // all_of
#include <unordered_map>

#include "netcode/detail/handler.hh"
#include "netcode/detail/repair.hh"
#include "netcode/detail/serializer.hh"
#include "netcode/detail/source.hh"

namespace ntc { namespace detail {

/*------------------------------------------------------------------------------------------------*/

/// @internal
class reconstruct final
{
public:

  reconstruct(handler_base& h)
    : handler_(h)
    , sources_{}
    , repairs_{}
    , src_to_repairs_{}
  {}

  void
  add(source&& src)
  {
    // Ask user to read the bytes of this new source.
    handler_.on_ready_symbol(src.user_size(), src.buffer().data());

    // First, look for all repairs that contain this incoming source in order to remove it
    // from these repairs.
    const auto search_range = src_to_repairs_.equal_range(src.id());
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
    }

    // Now, there are no more repairs that reference the current source, thus we now update the
    // mapping src -> repairs.
    src_to_repairs_.erase(src.id());

    // Finally, insert this new source in the set of known sources.
    const auto src_id = src.id(); // to force evaluation order in the following call.
    sources_.emplace(src_id, std::move(src));
  }

  void
  add(repair&& r)
  {
    /// First check if r is useless. Indeed, if all sources it references were correctly received,
    /// then it's useless to remove them from this repair, which is a costly operation.
    const auto useless = std::all_of( r.source_ids().begin(), r.source_ids().end()
                                    , [this](std::uint32_t src_id)
                                      {
                                        return sources_.count(src_id);
                                      });
    if (useless)
    {
      return;
    }

    // Add this repair to the set of known repairs.
    const auto r_id = r.id(); // to force evaluation order in the following call.
    const auto insertion = repairs_.emplace(r_id, std::move(r));
    assert(insertion.second && "Repair with the same id already processed");

    auto* r_ptr = &insertion.first->second;

    // Reverse loop as vector::erase() invalidates iterators past the one being erased.
    for ( auto id_rcit = r_ptr->source_ids().rbegin(), end = r_ptr->source_ids().rend()
        ; id_rcit != end; ++id_rcit)
    {
      if (sources_.count(*id_rcit))
      {
        /// @todo remove src from r.

        // Get the iterator corresponding to the current reverse iterator.
        const auto to_erase = std::next(id_rcit).base();
        r_ptr->source_ids().erase(to_erase);
      }
      else
      {
        // Link this repair with the sources it references.
        src_to_repairs_.emplace(*id_rcit, r_ptr);
      }
    }
    assert(not r_ptr->source_ids().empty());

  }

private:

  /// @brief
  handler_base& handler_;

  /// @brief
  std::unordered_map<std::uint32_t, source> sources_;

  /// @brief
  std::unordered_map<std::uint32_t, repair> repairs_;

  /// @brief
  std::unordered_multimap<std::uint32_t, repair*> src_to_repairs_;
};

/*------------------------------------------------------------------------------------------------*/

}} // namespace ntc::detail
