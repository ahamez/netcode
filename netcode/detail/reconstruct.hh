#pragma once

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

    // Finally, insert this new source in the set of sources.
    const auto src_id = src.id(); // to force evaluation order in the following call.
    sources_.emplace(src_id, std::move(src));
  }

  void
  add(repair&&)
  {

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
