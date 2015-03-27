#pragma once

#include <algorithm>  // all_of, is_sorted
#include <cassert>
#include <vector>

#include <boost/container/map.hpp>
#include <boost/container/flat_set.hpp>

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

  /// @brief
  using missing_sources_type = boost::container::map< std::uint32_t
                                                    , std::vector<repairs_set_type::iterator>>;

  /// @brief
  decoder(unsigned int galois_field_size, std::function<void(const source&)> h)
    : gf_{galois_field_size}
    , callback_(h)
    , repairs_{}
    , sources_{}
    , missing_sources_{}
    , nb_useless_repairs_{0}
    , coefficients_{16}
    , inv_{16}
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
    const auto search = missing_sources_.find(src.id());
    if (search != missing_sources_.end())
    {
      for (auto r_cit : search->second)
      {
        auto& r = r_cit->second;
        remove_source_from_repair(src, r);

        if (r.source_ids().size() == 1)
        {
          // Create missing source and give it back to the decoder.
          (*this)(create_source_from_repair(r));

          // Source reconstructed, we can now safely erase the current repair as it is no longer
          // useful.
          repairs_.erase(r_cit);
        }
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
  operator()(repair&& incoming_r)
  {
    // By construction, the list of source identifiers should be sorted.
    assert(not incoming_r.source_ids().empty());
    assert(std::is_sorted(begin(incoming_r.source_ids()), end(incoming_r.source_ids())));

    // Remove sources with an id smaller than the smallest the current repair encodes.
    // Remove repairs which encodes sources with an id smaller than the smallest the current repair
    /// encodes.
    drop_outdated(incoming_r.source_ids().front());

    /// Check if incoming_r is useless. Indeed, if all sources it references were correctly
    /// received, then it's useless to remove them from this repair, which is a costly operation.
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

    // Reverse loop as vector::erase() invalidates iterators past the one being erased.
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
          missing_sources_.emplace( *id_rcit
                                  , std::vector<repairs_set_type::iterator>{r_cit});
        }
        else
        {
          search_src_id->second.emplace_back(r_cit);
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

      // Give back the decode source to the decoder.
      (*this)(std::move(src));

      // This repair is no longer needed.
      repairs_.erase(r_cit);
      return;
    }

    attempt_full_decoding();
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
    if (missing_sources_.size() <= repairs().size())
    {
      // Build coefficient matrix.
      coefficients_.resize(repairs_.size());

      // Invert it.
      inv_.resize(coefficients_.dimension());
      const auto r_col = invert(gf_, coefficients_, inv_);
      if (r_col != inverted_pos)
      {
        // Inversion failed, remove the faulty repair.
      }
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

    // We need a total order on repairs_set_type::iterator.
    struct cmp
    {
      bool
      operator()(const repairs_set_type::iterator& lhs, const repairs_set_type::iterator& rhs)
      const noexcept
      {
        return lhs->first < rhs->first;
      }
    };

    // Then, remove repairs which references this id.
    // We can't delete repairs on the fly as they are referenced by several other missing sources.
    boost::container::flat_set<repairs_set_type::iterator, cmp> repairs_to_erase;
    repairs_to_erase.reserve(32);
    for (auto cit = missing_sources_.begin(); cit != missing_lb; ++cit)
    {
      for (const auto& r_cit : cit->second)
      {
        const auto& r = r_cit->second;
        assert(not r.source_ids().empty());
        assert(    (r.source_ids().front() < id  and r.source_ids().back() < id)
                or (r.source_ids().front() >= id and r.source_ids().back() >= id)
              );
        if (r.source_ids().back() < id)
        {
          // We found a repair for which all encoded sources have an identifier smaller than id,
          // thus it is outdated.
          repairs_to_erase.insert(r_cit);
        }
      }
    }

    sources_.erase(sources_.begin(), sources_.lower_bound(id));
    missing_sources_.erase(missing_sources_.begin(), missing_lb);
    for (const auto r_cit : repairs_to_erase)
    {
      repairs_.erase(r_cit);
    }
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

private:

  /// @brief The implementation of a Galois field.
  galois_field gf_;

  /// @brief The callback to call when a source has been decoded.
  std::function<void(const source&)> callback_;

  /// @brief The set of received repairs.
  repairs_set_type repairs_;

  /// @brief The set of received sources.
  sources_set_type sources_;

  /// @brief All sources that have not been yet received, but which are referenced by a repair.
  missing_sources_type missing_sources_;

  /// @brief The number of repairs which were dropped because they were useless.
  std::size_t nb_useless_repairs_;

  /// @brief Re-use the same memory for the matrix of coefficients.
  square_matrix coefficients_;

  /// @brief Re-use the same memory for the inverted matrix of coefficients.
  square_matrix inv_;
};

/*------------------------------------------------------------------------------------------------*/

}} // namespace ntc::detail
