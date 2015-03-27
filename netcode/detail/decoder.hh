#pragma once

#include <algorithm>  // all_of, is_sorted
#include <cassert>
#include <unordered_map>

#include "netcode/detail/coefficient.hh"
#include "netcode/detail/galois_field.hh"
#include "netcode/detail/multiple.hh"
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
    , latest_source_{-1}
  {}

  /// @brief What to do when a source is received or decoded.
  void
  operator()(source&& src)
  {
//    // Check if this source has already been received or decoded.
//    if (src.id() <= latest_source_)
//    {
//      return;
//    }

    // Notify netcode::encoder.
    callback_(src);

    // First, look for all repairs that contain this incoming source in order to remove it.
    const auto search_range = missing_sources_.equal_range(src.id());
    for (auto cit = search_range.first; cit != search_range.second; ++cit)
    {
      // A reference to the current repair.
      auto& r = *cit->second;

      remove_source_from_repair(src, r);

      if (r.source_ids().size() == 1)
      {
        // Create missing source and give it back to the decoder.
        (*this)(create_source_from_repair(r));

        // Source reconstructed, we can now safely erase the current repair as it is no longer
        // useful.
        repairs_.erase(r.id());
      }
    }

    // Now, there are no more repairs that reference the current source, thus we now update the
    // mapping src -> repairs.
    missing_sources_.erase(src.id());

    // Finally, insert-move this new source in the set of known sources.
    const auto src_id = src.id(); // to force evaluation order in the following call.
    sources_.emplace(src_id, std::move(src));

    /// @todo Check that it's possible to combine several repairs in order to reconstruct
    /// missing sources.
  }

  /// @brief What to do when a repair is received.
  void
  operator()(repair&& r)
  {
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
      // Create missing source and give it back to the decoder.
      (*this)(create_source_from_repair(*r_ptr));

      // This repair is no longer needed.
      repairs_.erase(r_ptr->id());
      return;
    }

    /// @todo Check that it's possible to combine several repairs in order to reconstruct
    /// missing sources.
  }

  /// @brief Decode a source contained in a repair.
  /// @attention @p r shall encode exactly one source.
  source
  create_source_from_repair(const repair& r)
  noexcept
  {
    assert(r.source_ids().size() == 1 && "Repair encodes more that 1 source");
    const auto src_id = r.source_ids().front();

    // The inverse of the coefficient which was used to encode the missing source.
    const auto inv = gf_.divide(1, coefficient(gf_, r.id(), src_id));

    // Reconstruct size.
    const auto src_sz = gf_.multiply(static_cast<std::uint32_t>(r.size()), inv);

    // The source that will be reconstructed.
    source src{src_id, byte_buffer(make_multiple(src_sz, 16)), src_sz};

    // Reconstruct missing source.
    gf_.multiply(r.buffer().data(), src.buffer().data(), src_sz, inv);

    return src;
  }

  /// @brief
  void
  remove_source_from_repair(const source& src, repair& r)
  noexcept
  {
    assert(r.source_ids().size() > 1 && "Repair encodes only one source");

    const auto coeff = coefficient(gf_, r.id(), src.id());

    // Remove source size.
    r.size() ^= gf_.multiply(coeff, static_cast<std::uint32_t>(src.user_size()));

    // Remove symbol.
    gf_.multiply_add(src.buffer().data(), r.buffer().data(), src.user_size(), coeff);

    // Remove src id of the list of the current repair source identifiers.
    const auto id_search = std::find(begin(r.source_ids()), end(r.source_ids()), src.id());
    assert(id_search != end(r.source_ids()) && "Source id not in current repair");
    r.source_ids().erase(id_search);
  }

private:

  ///
  galois_field gf_;

  /// @brief The callback to call when a source has been decoded.
  std::function<void(const source&)> callback_;

  /// @brief The set of received sources.
  std::unordered_map<std::uint32_t, source> sources_;

  /// @brief The set of received repairs.
  std::unordered_map<std::uint32_t, repair> repairs_;

  /// @brief All sources that have not been yet received, but which are referenced by a repair.
  std::unordered_multimap<std::uint32_t, repair*> missing_sources_;

  ///
  std::int64_t latest_source_;
};

/*------------------------------------------------------------------------------------------------*/

}} // namespace ntc::detail
