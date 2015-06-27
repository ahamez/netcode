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
                , in_order order)
  : m_gf{galois_field_size}
  , m_in_order{order == in_order::yes ? true : false}
  , m_first_missing_source_in_order{0}
  , m_ordered_sources{}
  , m_callback(h)
  , m_repairs{}
  , m_sources{}
  , m_last_id{nullptr}
  , m_missing_sources{}
  , m_nb_useless_repairs{0}
  , m_nb_failed_full_decodings{0}
  , m_nb_decoded{0}
  , m_coefficients{32}
  , m_inv{32}
{}

/*------------------------------------------------------------------------------------------------*/

void
decoder::operator()(source&& src)
{
  if (m_last_id and src.id() < *m_last_id)
  {
    // This source has already been seen in the past.
    return;
  }

  if (m_sources.count(src.id()))
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
  if (m_last_id and *(incoming_r.source_ids().end() - 1) < *m_last_id)
  {
    // It's a repair that provide outdated informations, drop it.
    return;
  }

  if (m_repairs.count(incoming_r.id()))
  {
    // A duplicate repair. Drop it.
    return;
  }

  // Remove sources with an id strictly less than the smallest the current repair encodes.
  // Remove repairs which encodes sources with an id smaller than the smallest the current repair
  // encodes.
  drop_outdated(*incoming_r.source_ids().begin());

  // Check if incoming_r is useless. Indeed, if all sources it references were correctly
  // received, then it's useless to remove them from this repair, which is a costly operation.
  const auto useless = std::all_of( begin(incoming_r.source_ids()), end(incoming_r.source_ids())
                                  , [this](std::uint32_t src_id)
                                    {
                                      return m_sources.count(src_id);
                                    });
  if (useless)
  {
    // Drop repair.
    ++m_nb_useless_repairs;
    return;
  }

  // Add this repair to the set of known repairs.
  const auto r_id = incoming_r.id(); // to force evaluation order in the following call.
  const auto insertion = m_repairs.emplace(r_id, std::move(incoming_r));
  assert(insertion.second && "Repair with the same id already processed");

  // Don't use incoming_r beyond this point (as it was moved into repairs_), instead use r.
  auto r_cit = insertion.first;
  auto& r = insertion.first->second;

  // Reverse loop as flat_set::erase() invalidates iterators behind the one being erased.
  for ( auto id_rcit = r.source_ids().rbegin(), end = r.source_ids().rend(); id_rcit != end
      ; ++id_rcit)
  {
    const auto search = m_sources.find(*id_rcit);
    if (search != m_sources.end())
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
      auto search_src_id = m_missing_sources.find(*id_rcit);
      if (search_src_id == m_missing_sources.end())
      {
        // Create missing source.
        m_missing_sources.emplace(*id_rcit, repairs_iterators_type{r_cit});
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
    m_missing_sources.erase(src.id());

    // This repair is no longer needed.
    m_repairs.erase(r_cit);

    // This newly decode source might trigger the decoding of several other sources.
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
  const auto inv = m_gf.invert(m_gf.coefficient(r.id(), src_id));

  // Reconstruct size.
  const auto src_sz = m_gf.multiply_size(r.encoded_size(), inv);

  // The source that will be reconstructed.
  source src{src_id, byte_buffer(src_sz)};

  // Reconstruct missing source.
  m_gf.multiply(r.buffer().data(), src.buffer().data(), src_sz, inv);

  m_nb_decoded += 1;

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
  return m_repairs;
}

/*------------------------------------------------------------------------------------------------*/

const decoder::sources_set_type&
decoder::sources()
const noexcept
{
  return m_sources;
}

/*------------------------------------------------------------------------------------------------*/

const decoder::missing_sources_type&
decoder::missing_sources()
const noexcept
{
  return m_missing_sources;
}

/*------------------------------------------------------------------------------------------------*/

std::size_t
decoder::nb_useless_repairs()
const noexcept
{
  return m_nb_useless_repairs;
}

/*------------------------------------------------------------------------------------------------*/

std::size_t
decoder::nb_failed_full_decodings()
const noexcept
{
  return m_nb_failed_full_decodings;
}

/*------------------------------------------------------------------------------------------------*/

std::size_t
decoder::nb_decoded()
const noexcept
{
  return m_nb_decoded;
}

/*------------------------------------------------------------------------------------------------*/

void
decoder::add_source_recursive(source&& src)
{
  if (not m_in_order)
  {
    m_callback(src);
  }
  else if (src.id() == m_first_missing_source_in_order)
  {
    m_callback(src);
    m_first_missing_source_in_order += 1;
    // Send all sources that could not be previously sent because their ids were greater than
    // first_missing_source_.
    flush_ordered_sources();
  }

  // First, look for all repairs that encode this source.
  const auto search = m_missing_sources.find(src.id());
  if (search != m_missing_sources.end())
  {
    for (auto r_cit : search->second)
    {
      auto& r = r_cit->second;
      remove_source_from_repair(src, r);
    }

    // It's no longer a missing source.
    m_missing_sources.erase(search);
  }

  // Now, look for repairs that encodes only 1 source. When one is found, the corresponding
  // encoded source is decoded and the repair is erased.
  for ( auto miss_cit = m_missing_sources.begin(), miss_end = m_missing_sources.end()
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
        assert(m_last_id ? *r.source_ids().begin() >= *m_last_id : true);
        // Check that this source doesn't belong to the set of current sources.
        assert(not m_sources.count(*r.source_ids().begin()));

        // This missing source is referenced by only 1 repair and this repair references only 1
        // missing source. Thus, we can reconstruct the missing source.
        auto decoded_src = create_source_from_repair(r);

        // We can erase this repair.
        m_repairs.erase(r_cit);

        // It's no longer a missing source.
        miss_cit = m_missing_sources.erase(miss_cit);

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
  const auto insertion = m_sources.emplace(src_id, std::move(src));
  assert(insertion.second && "source already added");

  if (m_in_order and insertion.first->second.id() > m_first_missing_source_in_order)
  {
    // We can't send the current source as there are some older sources which have not been sent.
    m_ordered_sources.emplace(insertion.first->second.id(), &insertion.first->second);
  }
}

/*------------------------------------------------------------------------------------------------*/

void
decoder::drop_outdated(std::uint32_t id)
noexcept
{
  // All sources with an identifier strictly less than last_id_ are now considered outdated.
  if (not m_last_id)
  {
    m_last_id = std::unique_ptr<std::uint32_t>(new std::uint32_t{id});
  }
  else
  {
    *m_last_id = id;
  }

  // Remove repairs which references this id.
  for (auto cit = m_repairs.begin(), end = m_repairs.end(); cit != end;)
  {
    const auto& r = cit->second;
    assert(not r.source_ids().empty());

    // Does this repair encodes sources with identifiers strictly less than id? If so, it's no
    // longer useful.
    if (id > *r.source_ids().begin())
    {
      cit = m_repairs.erase(cit);
    }
    else
    {
       ++cit;
    }
  }

  if (m_in_order)
  {
    // flush_ordered_sources() won't give to user sources with identifier smaller than id, thus we
    // take care of it now.
    for (auto cit = m_ordered_sources.begin(), end = m_ordered_sources.lower_bound(id); cit != end;)
    {
      m_callback(*cit->second);
      cit = m_ordered_sources.erase(cit);
    }
    if (m_first_missing_source_in_order < id)
    {
      m_first_missing_source_in_order = id;
    }
    flush_ordered_sources();
  }

  // Erase all sources and missing sources with an identifer smaller (strict) than id.
  m_sources.erase(m_sources.begin(), m_sources.lower_bound(id));
  m_missing_sources.erase(m_missing_sources.begin(), m_missing_sources.lower_bound(id));
}

/*------------------------------------------------------------------------------------------------*/

void
decoder::remove_source_data_from_repair(const source& src, repair& r)
noexcept
{
  assert(r.source_ids().size() > 1 && "Repair encodes only one source");
  assert(src.size() <= r.buffer().size());

  const auto coeff = m_gf.coefficient(r.id(), src.id());

  // Remove source size.
  r.encoded_size()
    = static_cast<std::uint16_t>(m_gf.multiply_size(src.size(), coeff) ^ r.encoded_size());

  // Remove symbol.
  m_gf.multiply_add(src.buffer().data(), r.buffer().data(), src.size(), coeff);
}

/*------------------------------------------------------------------------------------------------*/

void
decoder::attempt_full_decoding()
{
  if (m_repairs.empty() or m_missing_sources.empty())
  {
    return;
  }

  // Do we have enough repairs to try to decode missing sources?
  if (m_missing_sources.size() > m_repairs.size())
  {
    return;
  }

  assert(m_missing_sources.size() == m_repairs.size() && "More repairs than missing sources");
  assert(m_missing_sources.size() > 1 && "Trying to create a matrix for only one missing source.");

  // Build coefficient matrix.
  m_coefficients.resize(m_repairs.size());
  auto col = 0ul;
  for (const auto& r : m_repairs)
  {
    auto row = 0ul;
    for (const auto& missing : m_missing_sources)
    {
      m_coefficients(row, col) = r.second.source_ids().count(missing.first)
                               ? m_gf.coefficient(r.first, missing.first)
                               : 0u; // repair doesn't encode the missing source.
      ++row;
    }
    ++col;
  }

  // Invert it.
  m_inv.resize(m_coefficients.dimension());
  const auto r_col = invert(m_gf, m_coefficients, m_inv);
  if (r_col.first)
  {
    // Inversion failed, remove the faulty repair.
    m_nb_failed_full_decodings += 1;

    // Find the repair corresponding to r_col.
    auto r_cit = m_repairs.begin();
    // To avoid a conversion warning with clang's -Wconversion.
    using difference_type = std::iterator_traits<decltype(r_cit)>::difference_type;
    std::advance(r_cit, static_cast<difference_type>(r_col.second));

    // Remove repair from missing sources that reference it.
    for (const auto src : r_cit->second.source_ids())
    {
      m_missing_sources[src].erase(r_cit);
    }

    // We can now effectively remove the repair.
    m_repairs.erase(r_cit);
    return;
  }

  // Matrix successfully inverted, we can now decode missing sources. Phew!

  // Build an index for fast retreiving of repairs from the inverted matrix.
  std::vector<repair*> index;
  index.reserve(m_repairs.size());
  for (auto& rid_repair : m_repairs)
  {
    index.emplace_back(&rid_repair.second);
  }

  auto src_col = 0u;
  for (const auto& miss : m_missing_sources)
  {
    // First, decode the size of the source.
    const auto src_sz = [&,this]
    {
      std::uint16_t res = 0;
      for (auto repair_row = 0ul; repair_row < m_inv.dimension(); ++repair_row)
      {
        const auto coeff = m_inv(repair_row, src_col);
        if (coeff != 0)
        {
          const auto tmp = m_gf.multiply_size(index[repair_row]->encoded_size(), coeff);
          res = static_cast<std::uint16_t>(tmp) ^ res;
        }
      }
      return res;
    }();

    // Now, decode symbol.
    source src{miss.first, byte_buffer(src_sz, 0 /* zero out the buffer */)};
    auto repair_row = 0ul;
    auto coeff = 0u;

    // Find first non-zero coefficient.
    for (; repair_row < repairs().size(); ++repair_row)
    {
      coeff = m_inv(repair_row, src_col);
      if (coeff != 0)
      {
        break;
      }
    }
    assert(repair_row != m_repairs.size() && "No coefficients for missing source");

    // Repair's buffer might be smaller than the size of the source to decode, or it could be
    // the opposite situation. Thus, we need to make sure that we only read the right number of
    // bytes.
    auto sz = std::min(src_sz, static_cast<std::uint16_t>(index[repair_row]->buffer().size()));
    m_gf.multiply(index[repair_row]->buffer().data(), src.buffer().data(), sz, coeff);

    for (++repair_row; repair_row < m_inv.dimension(); ++repair_row)
    {
      coeff = m_inv(repair_row, src_col);
      if (coeff != 0)
      {
        sz = std::min(src_sz, static_cast<std::uint16_t>(index[repair_row]->buffer().size()));
        m_gf.multiply_add(index[repair_row]->buffer().data(), src.buffer().data(), sz, coeff);
      }
    }
    ++src_col;

    // Source decoded, add it to the set of known sources.
    const auto insertion = m_sources.emplace(miss.first, std::move(src));
    assert(insertion.second && "source already added");

    const auto& inserted_src = insertion.first->second;
    if (not m_in_order)
    {
      m_callback(inserted_src);
    }
    else if (inserted_src.id() == m_first_missing_source_in_order)
    {
      m_callback(inserted_src);
      m_first_missing_source_in_order += 1;
      // Send all sources that could not be previously sent because their ids were greater than
      // first_missing_source_.
      flush_ordered_sources();
    }
    else
    {
      // We can't send the current source as there are some older sources which have not been sent.
      m_ordered_sources.emplace(inserted_src.id(), &inserted_src);
    }
  }

  m_nb_decoded += m_missing_sources.size();

  // Cleanup.
  m_repairs.clear();
  m_missing_sources.clear();
}

/*------------------------------------------------------------------------------------------------*/

void
decoder::flush_ordered_sources()
{
  // If we find the first missing source, we can give to user all sources with a identifier
  // that follow m_first_missing_source in sequence.
  auto cit = m_ordered_sources.find(m_first_missing_source_in_order);
  if (cit == m_ordered_sources.end())
  {
    return;
  }
  while (true)
  {
    assert(cit->first == m_first_missing_source_in_order);
    m_callback(*cit->second);
    cit = m_ordered_sources.erase(cit);
    m_first_missing_source_in_order += 1;

    if (cit == m_ordered_sources.end() or cit->first > m_first_missing_source_in_order)
    {
      break;
    }
  }
}

/*------------------------------------------------------------------------------------------------*/

}} // namespace ntc::detail
