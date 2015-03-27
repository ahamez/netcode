#pragma once

#include <algorithm>  // all_of, is_sorted
#include <cassert>
#include <map>
#include <set>

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
    , repairs_{}
    , sources_{}
    , missing_sources_{}
    , nb_useless_repairs_{0}
  {}

  /// @brief What to do when a source is received or decoded.
  void
  operator()(source&& src)
  {
    if (sources_.count(src.id()))
    {
      // This source exists in the set of received sources.
      return;
    }

    if (not missing_sources().empty() and src.id() < missing_sources_.begin()->first)
    {
      // This source has an id smaller than the id of the oldest missing source, thus it means
      // that it was already decoded, received or abandoned.
      return;
    }

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

    attempt_full_decoding();
  }

  /// @brief What to do when a repair is received.
  void
  operator()(repair&& r)
  {
    // By construction, the list of source identifiers should be sorted.
    assert(not r.source_ids().empty());
    assert(std::is_sorted(begin(r.source_ids()), end(r.source_ids())));

    // Remove sources with an id smaller than the smallest the current repair encodes.
    // Remove repairs which encodes sources with an id smaller than the smallest the current repair
    /// encodes.
    drop_outdated(r.source_ids().front());

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
      ++nb_useless_repairs_;
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
      const auto search = sources_.find(*id_rcit);
      if (search != sources_.end())
      {
        // The source has already been received.
        remove_source_data_from_repair(search->second /* source */, *r_ptr);

        // Get the 'normal' iterator corresponding to the current reverse iterator.
        // http://stackoverflow.com/a/1830240/21584
        const auto to_erase = std::next(id_rcit).base();
        r_ptr->source_ids().erase(to_erase);
      }
      else
      {
        // Link this repair with the missing sources it references.
        missing_sources_.emplace(*id_rcit, r_ptr);
      }
    }
    assert(not r_ptr->source_ids().empty());

    if (r_ptr->source_ids().size() == 1)
    {
      // Create missing source.
      auto src = create_source_from_repair(*r_ptr);

      // Source is no longer missing.
      missing_sources_.erase(src.id());

      // Give back the decode source to the decoder.
      (*this)(std::move(src));

      // This repair is no longer needed.
      repairs_.erase(r_ptr->id());
      return;
    }

    attempt_full_decoding();
  }

  /// @brief Try to construct missing sources from the set of repairs.
  void
  attempt_full_decoding()
  {
    if (repairs_.empty())
    {
      return;
    }
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

  /// @brief Remove a source from a repair, but not the id from the list of source identifiers.
  /// @attention The id of the removed src shall be removed from the repair's list of source
  /// identifiers.
  void
  remove_source_data_from_repair(const source& src, repair& r)
  noexcept
  {
    assert(r.source_ids().size() > 1 && "Repair encodes only one source");

    const auto coeff = coefficient(gf_, r.id(), src.id());

    // Remove source size.
    r.size() ^= gf_.multiply(coeff, static_cast<std::uint32_t>(src.user_size()));

    // Remove symbol.
    gf_.multiply_add(src.buffer().data(), r.buffer().data(), src.user_size(), coeff);
  }

  /// @brief Remove a source from a repair.
  void
  remove_source_from_repair(const source& src, repair& r)
  noexcept
  {
    remove_source_data_from_repair(src, r);
    // Remove src id of the list of the current repair source identifiers.
    const auto id_search = std::find(begin(r.source_ids()), end(r.source_ids()), src.id());
    assert(id_search != end(r.source_ids()) && "Source id not in current repair");
    r.source_ids().erase(id_search);
  }

  /// @brief Drop outdated sources and repairs.
  void
  drop_outdated(std::uint32_t id)
  noexcept
  {
    // First, find the upper bound of missing sources with an identifier smaller than id.
    const auto missing_lb = missing_sources_.lower_bound(id);

    // Then, remove repairs which references this id.
    // We can't delete repairs on the fly as they are referenced by several missing sources.
    std::set<std::uint32_t> repairs_to_erase;
    for (auto cit = missing_sources_.begin(); cit != missing_lb; ++cit)
    {
      const auto& r = *cit->second;
      assert(not r.source_ids().empty());
      assert(    (r.source_ids().front() < id and r.source_ids().back() < id)
              or (r.source_ids().front() >= id and r.source_ids().back() >= id)
            );
      if (r.source_ids().back() < id)
      {
        // We found a repair for which all encoded sources have an identifier smaller than id,
        // thus it is outdated.
        repairs_to_erase.insert(r.id());
      }
    }

    sources_.erase(sources_.begin(), sources_.lower_bound(id));
    missing_sources_.erase(missing_sources_.begin(), missing_lb);
    for (const auto repair_id : repairs_to_erase)
    {
      repairs_.erase(repair_id);
    }
  }

  /// @brief Get the current set of repairs, indexed by identifier.
  const std::map<std::uint32_t, repair>&
  repairs()
  const noexcept
  {
    return repairs_;
  }

  /// @brief Get the current set of sources, indexed by identifier.
  const std::map<std::uint32_t, source>&
  sources()
  const noexcept
  {
    return sources_;
  }

  /// @brief Get the current set of missing sources,
  const std::multimap<std::uint32_t, repair*>&
  missing_sources()
  const noexcept
  {
    return missing_sources_;
  }

  /// @brief Get the number of repairs that were dropped because they useless.
  std::size_t
  nb_useless_repairs()
  const noexcept
  {
    return nb_useless_repairs_;
  }

private:

  ///
  galois_field gf_;

  /// @brief The callback to call when a source has been decoded.
  std::function<void(const source&)> callback_;

  /// @brief The set of received repairs.
  std::map<std::uint32_t, repair> repairs_;

  /// @brief The set of received sources.
  std::map<std::uint32_t, source> sources_;

  /// @brief All sources that have not been yet received, but which are referenced by a repair.
  std::multimap<std::uint32_t, repair*> missing_sources_;

  ///
  std::size_t nb_useless_repairs_;
};

/*------------------------------------------------------------------------------------------------*/

}} // namespace ntc::detail
