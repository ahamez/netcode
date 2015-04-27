#include <algorithm>  // all_of
#include <cassert>
#include <vector>

#include "netcode/detail/decoder.hh"
#include "netcode/detail/invert_matrix.hh"
#include "netcode/detail/repair.hh"
#include "netcode/detail/source.hh"

namespace ntc { namespace detail {

/*------------------------------------------------------------------------------------------------*/

decoder::decoder( std::uint8_t galois_field_size, std::function<void(const source&)> h
                , bool in_order)
  : gf_{galois_field_size}
  , in_order_{in_order}
  , first_missing_source_{0}
  , ordered_sources_{}
  , callback_(h)
  , repairs_{}
  , sources_{}
  , last_id_{}
  , missing_sources_{}
  , nb_useless_repairs_{0}
  , nb_failed_full_decodings_{0}
  , nb_decoded_{0}
  , coefficients_{32}
  , inv_{32}
{}

/*------------------------------------------------------------------------------------------------*/

void
decoder::operator()(source&& src)
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

/*------------------------------------------------------------------------------------------------*/

void
decoder::operator()(repair&& incoming_r)
{
  // By construction, the list of source identifiers should be empty.
  assert(not incoming_r.source_ids().empty());

  //                     last id in source_ids
  //               ------------------------------------
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

/*------------------------------------------------------------------------------------------------*/

source
decoder::create_source_from_repair(const repair& r)
noexcept
{
  assert(r.source_ids().size() == 1 && "Repair encodes more that 1 source");
  const auto src_id = *r.source_ids().begin();

  // The inverse of the coefficient which was used to encode the missing source.
  const auto inv = gf_.invert(gf_.coefficient(r.id(), src_id));

  // Reconstruct size.
  const auto src_sz = gf_.multiply_size(r.encoded_size(), inv);

  // The source that will be reconstructed.
  source src{src_id, byte_buffer(src_sz), src_sz};

  // Reconstruct missing source.
  gf_.multiply(r.buffer().data(), src.buffer().data(), src_sz, inv);

  nb_decoded_ += 1;

  return src;
}

/*------------------------------------------------------------------------------------------------*/

void
decoder::remove_source_from_repair(const source& src, repair& r)
noexcept
{
  remove_source_data_from_repair(src, r);
  // Remove src id of the list of the current repair source identifiers.
  const auto id_search = std::find(begin(r.source_ids()), end(r.source_ids()), src.id());
  assert(id_search != end(r.source_ids()) && "Source id not in current repair");
  r.source_ids().erase(id_search);
}

/*------------------------------------------------------------------------------------------------*/

const decoder::repairs_set_type&
decoder::repairs()
const noexcept
{
  return repairs_;
}

/*------------------------------------------------------------------------------------------------*/

const decoder::sources_set_type&
decoder::sources()
const noexcept
{
  return sources_;
}

/*------------------------------------------------------------------------------------------------*/

const decoder::missing_sources_type&
decoder::missing_sources()
const noexcept
{
  return missing_sources_;
}

/*------------------------------------------------------------------------------------------------*/

std::size_t
decoder::nb_useless_repairs()
const noexcept
{
  return nb_useless_repairs_;
}

/*------------------------------------------------------------------------------------------------*/

std::size_t
decoder::nb_failed_full_decodings()
const noexcept
{
  return nb_failed_full_decodings_;
}

/*------------------------------------------------------------------------------------------------*/

std::size_t
decoder::nb_decoded()
const noexcept
{
  return nb_decoded_;
}

/*------------------------------------------------------------------------------------------------*/

void
decoder::add_source_recursive(source&& src)
{
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
    // Does this missing source is referenced by only one repair?
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

        // This missing source is referenced by only 1 repair and this repair references only 1
        // missing source. Thus, we can reconstruct the missing source.
        auto decoded_src = create_source_from_repair(r);

        // We can erase this repair.
        repairs_.erase(r_cit);

        // It's no longer a missing source.
        const auto to_erase = miss_cit;
        ++miss_cit;
        missing_sources_.erase(to_erase);

        // This newly decoded source might trigger the decoding of other sources.
        add_source_recursive(std::move(decoded_src));
      }
      else
      {
        // This repair encodes too much sources, other sources are needed to reconstruct *miss_cit.
        // Try next missing source.
        ++miss_cit;
      }
    }
    else
    {
      // This missing source is referenced by multiple repairs. Try next missing source.
      ++miss_cit;
    }
  }

  // Insert-move this new source in the set of known sources.
  const auto src_id = src.id(); // to force evaluation order in the following call.
  const auto insertion = sources_.emplace(src_id, std::move(src));
  assert(insertion.second && "source already added");

  handle_source(&insertion.first->second);
}

/*------------------------------------------------------------------------------------------------*/

void
decoder::drop_outdated(std::uint32_t id)
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

  if (in_order_)
  {
    for (auto cit = ordered_sources_.begin(); cit != ordered_sources_.lower_bound(id);)
    {
      callback_(*cit->second);
      const auto to_erase = cit;
      ++cit;
      ordered_sources_.erase(to_erase);
    }
    if (first_missing_source_ < id)
    {
      first_missing_source_ = id;
    }
    flush_ordered_sources();
  }

  // Erase all sources and missing sources with an identifer smaller (strict) than id.
  sources_.erase(sources_.begin(), sources_.lower_bound(id));
  missing_sources_.erase(missing_sources_.begin(), missing_sources_.lower_bound(id));
}

/*------------------------------------------------------------------------------------------------*/

void
decoder::remove_source_data_from_repair(const source& src, repair& r)
noexcept
{
  assert(r.source_ids().size() > 1 && "Repair encodes only one source");
  assert(src.user_size() <= r.buffer().size());

  const auto coeff = gf_.coefficient(r.id(), src.id());

  // Remove source size.
  r.encoded_size()
    = static_cast<std::uint16_t>(gf_.multiply_size(src.user_size(), coeff) ^ r.encoded_size());

  // Remove symbol.
  gf_.multiply_add(src.buffer().data(), r.buffer().data(), src.user_size(), coeff);
}

/*------------------------------------------------------------------------------------------------*/

void
decoder::attempt_full_decoding()
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
      coefficients_(row, col) = r.second.source_ids().count(missing.first)
                              ? gf_.coefficient(r.first, missing.first)
                              : 0u; // repair doesn't encode the missing source.
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
    // To avoid a conversion warning with clang's -Wconversion.
    using difference_type = std::iterator_traits<decltype(r_cit)>::difference_type;
    std::advance(r_cit, static_cast<difference_type>(*r_col));

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
    const auto src_sz = [&,this]
    {
      std::uint16_t res = 0;
      for (auto repair_row = 0ul; repair_row < inv_.dimension(); ++repair_row)
      {
        const auto coeff = inv_(repair_row, src_col);
        if (coeff != 0)
        {
          const auto tmp = gf_.multiply_size(index[repair_row]->encoded_size(), coeff);
          res = static_cast<std::uint16_t>(tmp) ^ res;
        }
      }
      return res;
    }();

    // Now, decode symbol.
    source src{miss.first, byte_buffer(src_sz, 0 /* zero out the buffer */), src_sz};
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

    // Repair's buffer might be smaller than the size of the source to decode, or it could be
    // the opposite situation. Thus, we need to make sure that we only read the right number of
    // bytes.
    auto sz = std::min(src_sz, static_cast<std::uint16_t>(index[repair_row]->buffer().size()));
    gf_.multiply(index[repair_row]->buffer().data(), src.buffer().data(), sz, coeff);

    for (++repair_row; repair_row < inv_.dimension(); ++repair_row)
    {
      coeff = inv_(repair_row, src_col);
      if (coeff != 0)
      {
        sz = std::min(src_sz, static_cast<std::uint16_t>(index[repair_row]->buffer().size()));
        gf_.multiply_add(index[repair_row]->buffer().data(), src.buffer().data(), sz, coeff);
      }
    }
    ++src_col;

    // Source decoded, add it to the set of known sources.
    const auto insertion = sources_.emplace(miss.first, std::move(src));
    assert(insertion.second && "source already added");

    handle_source(&insertion.first->second);
  }

  nb_decoded_ += missing_sources_.size();

  // Cleanup.
  repairs_.clear();
  missing_sources_.clear();
}

/*------------------------------------------------------------------------------------------------*/

/// @brief
void
decoder::handle_source(const source* src)
{
  if (not in_order_)
  {
    callback_(*src);
  }
  else
  {
    if (src->id() == first_missing_source_)
    {
      callback_(*src);
      first_missing_source_ += 1;

      // Send all sources that could not be previously sent because their ids were greater than
      // first_missing_source_.
      flush_ordered_sources();
    }
    else
    {
      assert(src->id() > first_missing_source_);
      // We can't send the current source as they are some older sources which has not been sent
      // yet.
      ordered_sources_.emplace(src->id(), src);
    }
  }
}

/*------------------------------------------------------------------------------------------------*/

void
decoder::flush_ordered_sources()
{
  auto cit = ordered_sources_.find(first_missing_source_);
  while (cit != ordered_sources_.end())
  {
    assert(cit->second->id() == first_missing_source_);
    callback_(*cit->second);
    const auto to_erase = cit;
    ++cit;
    ordered_sources_.erase(to_erase);
    first_missing_source_ += 1;

    if (cit == ordered_sources_.end() or cit->second->id() > first_missing_source_)
    {
      break;
    }
  }
}

/*------------------------------------------------------------------------------------------------*/

}} // namespace ntc::detail
