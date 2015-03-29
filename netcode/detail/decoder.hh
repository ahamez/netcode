#pragma once

#include <algorithm>  // all_of, is_sorted
#include <cassert>
#include <vector>

#include <boost/container/flat_set.hpp>
#include <boost/container/map.hpp>
#include <boost/optional.hpp>

#include "netcode/detail/coefficient.hh"
#include "netcode/detail/galois_field.hh"
#include "netcode/detail/invert_matrix.hh"
#include "netcode/detail/multiple.hh"
#include "netcode/detail/repair.hh"
#include "netcode/detail/source.hh"
#include "netcode/detail/square_matrix.hh"

namespace ntc { namespace detail {

/*------------------------------------------------------------------------------------------------*/

/// @internal
/// @brief The component responsible for the decoding of detail::source from detail::repair.
class decoder final
{
public:

  /// @brief
  using repairs_set_type = boost::container::map<std::uint32_t, repair>;

  /// @brief
  using sources_set_type = boost::container::map<std::uint32_t, source>;

private:

  /// @brief We need a total order on repairs_set_type::iterator.
  struct cmp_repairs_iterator
  {
    bool
    operator()(const repairs_set_type::iterator& lhs, const repairs_set_type::iterator& rhs)
    const noexcept
    {
      // Use identifiers to sort.
      return lhs->first < rhs->first;
    }
  };

public:

  /// @brief
  using repairs_iterators_type = boost::container::flat_set< repairs_set_type::iterator
                                                           , cmp_repairs_iterator>;

  /// @brief
  using missing_sources_type = boost::container::map<std::uint32_t, repairs_iterators_type>;

  /// @brief Constructor.
  decoder(std::size_t galois_field_size, std::function<void(const source&)> h)
    : gf_{galois_field_size}
    , callback_(h)
    , repairs_{}
    , sources_{}
    , last_id_{}
    , missing_sources_{}
    , nb_useless_repairs_{0}
    , nb_failed_full_decodings_{0}
    , coefficients_{32}
    , inv_{32}
  {}

  /// @brief What to do when a source is received.
  void
  operator()(source&& src)
  {
    if (last_id_ and src.id() < *last_id_)
    {
      // This source has already been seen in the past.
      return;
    }

    if (sources_.count(src.id()))
    {
      // This source exists in the set of received sources.
      return;
    }

    add_source_recursive(std::move(src));

    attempt_full_decoding();
  }

  /// @brief What to do when a repair is received.
  void
  operator()(repair&& incoming_r)
  {
    // By construction, the list of source identifiers should be sorted.
    assert(not incoming_r.source_ids().empty());
    assert(std::is_sorted(begin(incoming_r.source_ids()), end(incoming_r.source_ids())));

    //                 last id in source_ids
    if (last_id_ and *(incoming_r.source_ids().end() - 1) < *last_id_)
    {
      // It's a repair that provide outdated informations, drop it.
      return;
    }

    if (repairs_.count(incoming_r.id()))
    {
      // A duplicate repair. Drop it.
      return;
    }

    // Remove sources with an id smaller than the smallest the current repair encodes.
    // Remove repairs which encodes sources with an id smaller than the smallest the current repair
    // encodes.
    drop_outdated(*incoming_r.source_ids().begin());

    // Check if incoming_r is useless. Indeed, if all sources it references were correctly
    // received, then it's useless to remove them from this repair, which is a costly operation.
    const auto useless = std::all_of( begin(incoming_r.source_ids()), end(incoming_r.source_ids())
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
    const auto r_id = incoming_r.id(); // to force evaluation order in the following call.
    const auto insertion = repairs_.emplace(r_id, std::move(incoming_r));
    assert(insertion.second && "Repair with the same id already processed");

    // Don't use incoming_r beyond this point (as it was moved into repairs_), instead use r.
    auto r_cit = insertion.first;
    auto& r = insertion.first->second;

    // Reverse loop as flat_set::erase() invalidates iterators past the one being erased.
    for ( auto id_rcit = r.source_ids().rbegin(), end = r.source_ids().rend(); id_rcit != end
        ; ++id_rcit)
    {
      const auto search = sources_.find(*id_rcit);
      if (search != sources_.end())
      {
        // The source has already been received.
        remove_source_data_from_repair(search->second /* source */, r);

        // Get the 'normal' iterator corresponding to the current reverse iterator.
        // http://stackoverflow.com/a/1830240/21584
        const auto to_erase = std::next(id_rcit).base();
        r.source_ids().erase(to_erase);
      }
      else
      {
        // Link this repair with the missing sources it references.
        auto search_src_id = missing_sources_.find(*id_rcit);
        if (search_src_id == missing_sources_.end())
        {
          // Create missing source.
          missing_sources_.emplace(*id_rcit, repairs_iterators_type{r_cit});
        }
        else
        {
          // The missing source already exists.
          search_src_id->second.emplace(r_cit);
        }
      }
    }
    assert(not r.source_ids().empty());

    if (r.source_ids().size() == 1)
    {
      // Create missing source.
      auto src = create_source_from_repair(r);

      // Source is no longer missing.
      missing_sources_.erase(src.id());

      // This repair is no longer needed.
      repairs_.erase(r_cit);

      // Give back the decoded source to the decoder.
      add_source_recursive(std::move(src));

      return;
    }

    attempt_full_decoding();
  }

  /// @brief Decode a source contained in a repair.
  /// @attention @p r shall encode exactly one source.
  source
  create_source_from_repair(const repair& r)
  noexcept
  {
    assert(r.source_ids().size() == 1 && "Repair encodes more that 1 source");
    const auto src_id = *r.source_ids().begin();

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

  /// @brief Get the current set of repairs, indexed by identifier.
  const repairs_set_type&
  repairs()
  const noexcept
  {
    return repairs_;
  }

  /// @brief Get the current set of sources, indexed by identifier.
  const sources_set_type&
  sources()
  const noexcept
  {
    return sources_;
  }

  /// @brief Get the current set of missing sources.
  const missing_sources_type&
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

  ///
  std::size_t
  nb_failed_full_decodings()
  const noexcept
  {
    return nb_failed_full_decodings_;
  }

private:

  /// @brief Recursively decode any repair that encodes only one source.
  void
  add_source_recursive(source&& src)
  {
    // Notify uppper decoder.
    callback_(src);

    // First, look for all repairs that encode this source.
    const auto search = missing_sources_.find(src.id());
    if (search != missing_sources_.end())
    {
      for (auto r_cit : search->second)
      {
        auto& r = r_cit->second;
        remove_source_from_repair(src, r);
      }

      // It's no longer a missing source.
      missing_sources_.erase(search);
    }

    // Now, look for repairs that encodes only 1 source. When one is found, the corresponding
    // encoded source is decoded and the repair is erased.
    for ( auto miss_cit = missing_sources_.begin(), miss_end = missing_sources_.end()
        ; miss_cit != miss_end;)
    {
      const auto& repairs_cits = miss_cit->second;
      // Does this missing source is references by multiple repairs?
      if (repairs_cits.size() == 1)
      {
        // Some shortcuts.
        const auto r_cit = *repairs_cits.begin();
        const auto& r = r_cit->second;

        // Does this repair encode only 1 source?
        if (r.source_ids().size() == 1)
        {
          // Check that this source wasn't decoded in the past.
          assert(last_id_ ? *r.source_ids().begin() >= *last_id_ : true);
          // Check that this source doesn't belong to the set of current sources.
          assert(not sources_.count(*r.source_ids().begin()));

          // This missing source is referenced by only 1 repair and this repair reference only 1
          // missing source. Thus, we can reconstruct the missing source.
          auto decoded_src = create_source_from_repair(r);

          // We can erase this repair.
          repairs_.erase(r_cit);

          // It's no longer a missing source.
          const auto to_erase = miss_cit;
          ++miss_cit;
          missing_sources_.erase(to_erase);

          // This newly decoded source might triger the decoding of other sources.
          add_source_recursive(std::move(decoded_src));
        }
        else
        {
          // This repair encodes too much sources. Try next missing source.
          ++miss_cit;
        }
      }
      else
      {
        // This missing source is referenced by multiple repairs. Try next missing source.
        ++miss_cit;
      }
    }

    // Finally, insert-move this new source in the set of known sources.
    const auto src_id = src.id(); // to force evaluation order in the following call.
    sources_.emplace(src_id, std::move(src));
  }

  /// @brief Drop outdated sources and repairs.
  void
  drop_outdated(std::uint32_t id)
  noexcept
  {
    // All sources with an identifier smaller than last_id_ are now considered outdated.
    last_id_ = id;

    // Remove repairs which references this id.
    for (auto cit = repairs_.begin(), end = repairs_.end(); cit != end;)
    {
      const auto& r = cit->second;
      assert(not r.source_ids().empty());
      assert(    (*r.source_ids().begin() <  id and *(r.source_ids().end() - 1) <  id)
              or (*r.source_ids().begin() >= id and *(r.source_ids().end() - 1) >= id));
      // Last source encoded by r has an identifier smaller than id.
      if (*(r.source_ids().end() - 1) < id)
      {
        const auto to_erase = cit;
        ++cit;
        repairs_.erase(to_erase);
      }
      else
      {
         ++cit;
      }
    }

    // Erase all sources and missing sources with an identifer smaller (strict) than id.
    sources_.erase(sources_.begin(), sources_.lower_bound(id));
    missing_sources_.erase(missing_sources_.begin(), missing_sources_.lower_bound(id));
  }

  /// @brief Remove a source from a repair, but not the id from the list of source identifiers.
  /// @attention The id of the removed src must be removed from the repair's list of source
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

  /// @brief Try to construct missing sources from the set of repairs.
  void
  attempt_full_decoding()
  {
    if (repairs_.empty() or missing_sources_.empty())
    {
      return;
    }

    // Do we have enough repairs to try to decode missing sources?
    if (missing_sources_.size() > repairs().size())
    {
      return;
    }

    assert(missing_sources_.size() == repairs().size() && "More repairs than missing sources");
    assert(missing_sources_.size() > 1 && "Trying to create a matrix for only one missing source.");

    // Build coefficient matrix.
    coefficients_.resize(repairs_.size());
    auto col = 0ul;
    for (const auto& r : repairs_)
    {
      auto row = 0ul;
      for (const auto& missing : missing_sources_)
      {
        coefficients_(row, col) = [&,this]
        {
          // Check if repair encodes the missing source.
          /// @todo Use the fact that source_id is sorted.
          const auto search_repair = std::find( r.second.source_ids().begin()
                                              , r.second.source_ids().end()
                                              , missing.first);
          return search_repair == r.second.source_ids().end()
               ? 0u // repair doesn't encode the missing source.
               : coefficient(gf_, r.first, missing.first);
        }();
        ++row;
      }
      ++col;
    }

    // Invert it.
    inv_.resize(coefficients_.dimension());
    if (const auto r_col = invert(gf_, coefficients_, inv_))
    {
      // Inversion failed, remove the faulty repair.
      ++nb_failed_full_decodings_;

      // Find the repair corresponding to r_col.
      auto r_cit = repairs_.begin();
      std::advance(r_cit, *r_col);

      // Remove repair from missing sources that reference it.
      for (const auto src : r_cit->second.source_ids())
      {
        missing_sources_[src].erase(r_cit);
      }

      // We can now effectively remove the repair.
      repairs_.erase(r_cit);
      return;
    }

    // Matrix successfully inverted, we can now decode missing sources. Phew!

    // Build an index for fast retreiving of repairs from the inv_ matrix.
    std::vector<repair*> index;
    index.reserve(repairs_.size());
    for (auto& rid_repair : repairs_)
    {
      index.emplace_back(&rid_repair.second);
    }

    auto src_col = 0u;
    for (const auto& miss : missing_sources_)
    {
      // First, decode the size of the source.
      const auto sz = [&,this]
      {
        auto res = 0ul;
        for (auto repair_row = 0ul; repair_row < inv_.dimension(); ++repair_row)
        {
          const auto coeff = inv_(repair_row, src_col);
          if (coeff != 0)
          {
            res ^= gf_.multiply(static_cast<std::uint32_t>(index[repair_row]->size()), coeff);
          }
        }
        return res;
      }();

      // Now, decode symbol.
      source src{miss.first, byte_buffer(make_multiple(sz, 16)), sz};
      auto repair_row = 0ul;
      auto coeff = 0u;

      // Find first non-zero coefficient.
      for (; repair_row < repairs().size(); ++repair_row)
      {
        coeff = inv_(repair_row, src_col);
        if (coeff != 0)
        {
          break;
        }
      }
      assert(repair_row != repairs_.size() && "No coefficients for missing source");

      gf_.multiply( index[repair_row]->buffer().data(), src.buffer().data(), src.buffer().size()
                  , coeff);

      for (++repair_row; repair_row < inv_.dimension(); ++repair_row)
      {
        coeff = inv_(repair_row, src_col);
        if (coeff != 0)
        {
          gf_.multiply_add( index[repair_row]->buffer().data(), src.buffer().data()
                          , src.buffer().size(), coeff);
        }
      }
      ++src_col;

      // Notify netcode::encoder.
      callback_(src);

      // Source decoded, add it to the set of known sources.
      sources_.emplace(miss.first, std::move(src));
    }

    // Cleanup.
    repairs_.clear();
    missing_sources_.clear();
  }

private:

  /// @brief The implementation of a Galois field.
  galois_field gf_;

  /// @brief The callback to call when a source has been decoded.
  std::function<void(const source&)> callback_;

  /// @brief The set of received repairs.
  repairs_set_type repairs_;

  /// @brief The set of received sources.
  sources_set_type sources_;

  /// @brief Remember the last source identifier.
  ///
  /// All sources with an identifier smaller than @p last_id_ were received or decoded in the past.
  boost::optional<std::uint32_t> last_id_;

  /// @brief All sources that have not been yet received, but which are referenced by a repair.
  missing_sources_type missing_sources_;

  /// @brief The number of repairs which were dropped because they were useless.
  std::size_t nb_useless_repairs_;

  ///
  std::size_t nb_failed_full_decodings_;

  /// @brief Re-use the same memory for the matrix of coefficients.
  square_matrix coefficients_;

  /// @brief Re-use the same memory for the inverted matrix of coefficients.
  square_matrix inv_;
};

/*------------------------------------------------------------------------------------------------*/

}} // namespace ntc::detail
