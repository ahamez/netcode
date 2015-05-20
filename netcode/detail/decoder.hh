#pragma once

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-parameter"
#pragma GCC diagnostic ignored "-Wconversion"
#pragma GCC diagnostic ignored "-Wshadow"
#include <boost/container/flat_set.hpp>
#include <boost/container/map.hpp>
#include <boost/optional.hpp>
#pragma GCC diagnostic pop

#include "netcode/detail/galois_field.hh"
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

  /// @brief Type of an ordered container of repairs.
  using repairs_set_type = boost::container::map<std::uint32_t, repair>;

  /// @brief Type of an ordered container of sources.
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

  /// @brief Type of a sorted container of repair iterators.
  using repairs_iterators_type = boost::container::flat_set< repairs_set_type::iterator
                                                           , cmp_repairs_iterator>;

  /// @brief Type of an ordered container of missing sources.
  using missing_sources_type = boost::container::map<std::uint32_t, repairs_iterators_type>;

public:

  /// @brief Constructor.
  decoder(std::uint8_t galois_field_size, std::function<void(const source&)> h, bool in_order);

  /// @brief What to do when a source is received.
  void
  operator()(source&& src);

  /// @brief What to do when a repair is received.
  void
  operator()(repair&& incoming_r);

  /// @brief Decode a source contained in a repair.
  /// @attention @p r shall encode exactly one source.
  source
  create_source_from_repair(const repair& r)
  noexcept;

  /// @brief Remove a source from a repair.
  /// @attention @p r shall encode more than one source.
  void
  remove_source_from_repair(const source& src, repair& r)
  noexcept;

  /// @brief Get the current set of repairs, indexed by identifier.
  const repairs_set_type&
  repairs()
  const noexcept;

  /// @brief Get the current set of sources, indexed by identifier.
  const sources_set_type&
  sources()
  const noexcept;

  /// @brief Get the current set of missing sources.
  const missing_sources_type&
  missing_sources()
  const noexcept;

  /// @brief Get the number of repairs that were dropped because they were useless.
  std::size_t
  nb_useless_repairs()
  const noexcept;

  /// @brief Get the number of times the full decoding failed.
  std::size_t
  nb_failed_full_decodings()
  const noexcept;

  /// @brief Get the number of decoded packets.
  std::size_t
  nb_decoded()
  const noexcept;

private:

  /// @brief Recursively decode any repair that encodes only one source.
  void
  add_source_recursive(source&& src);

  /// @brief Drop outdated sources and repairs.
  /// @param id The oldest id to keep. 
  ///
  /// All sources with an identifier smaller than @p id will be dropped, as well as all repairs
  /// that reference thes outdated sources.
  void
  drop_outdated(std::uint32_t id)
  noexcept;

  /// @brief Remove a source from a repair, but not the id from the list of source identifiers.
  /// @attention The id of the removed src must be removed from the repair's list of source
  /// identifiers afterwards.
  /// @attention @p r shall encode more than one source.
  void
  remove_source_data_from_repair(const source& src, repair& r)
  noexcept;

  /// @brief Try to construct missing sources from the set of repairs.
  void
  attempt_full_decoding();

  /// @brief Give sources to callback in order.
  /// @attention A pointer to the source is stored.
  void
  handle_source_in_order(const source*);

  /// @brief Give to callback ordered sources, if possible.
  void
  flush_ordered_sources();

private:

  /// @brief The implementation of a Galois field.
  galois_field m_gf;

  /// @brief Indicates if sources should be given in-order to the callback.
  const bool m_in_order;

  /// @brief The identifier of the first source which has not yet been given to callback.
  ///
  /// Used to give sources in-order to the callback.
  std::uint32_t m_first_missing_source;

  /// @brief Maintains a list of sources which could not be given to callback when some older
  /// sources are still missing.
  boost::container::map<std::uint32_t, const source*> m_ordered_sources;

  /// @brief The callback to call when a source has been decoded or received.
  const std::function<void(const source&)> m_callback;

  /// @brief The set of received repairs.
  repairs_set_type m_repairs;

  /// @brief The set of received sources.
  sources_set_type m_sources;

  /// @brief Remember the last source identifier.
  ///
  /// All sources with an identifier smaller than @p last_id_ were received or decoded in the past.
  boost::optional<std::uint32_t> m_last_id;

  /// @brief All sources that have not been yet received, but which are referenced by a repair.
  missing_sources_type m_missing_sources;

  /// @brief The number of repairs which were dropped because they were useless.
  std::size_t m_nb_useless_repairs;

  /// @brief The number of time a full decoding failed.
  std::size_t m_nb_failed_full_decodings;

  /// @brief The number of decoded sources.
  std::size_t m_nb_decoded;

  /// @brief Re-use the same memory for the matrix of coefficients.
  square_matrix m_coefficients;

  /// @brief Re-use the same memory for the inverted matrix of coefficients.
  square_matrix m_inv;
};

/*------------------------------------------------------------------------------------------------*/

}} // namespace ntc::detail
