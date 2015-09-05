#pragma once

#include <boost/optional.hpp>

#include "netcode/detail/galois_field.hh"
#include "netcode/detail/square_matrix.hh"

namespace ntc { namespace detail {

/*------------------------------------------------------------------------------------------------*/

/// @internal
/// @brief Invert a matrix using a Galois field.
/// @attention @p mat will be overwritten
/// @note This is the algorithm provided by jerasure ( http://jerasure.org )
/// @related square_matrix
/// @return A initialized optional value if inversion failed. In this case, the value is the column
/// which made the inversion fail.
boost::optional<std::size_t>
invert(galois_field& gf, square_matrix& mat, square_matrix& inv)
noexcept;

/*------------------------------------------------------------------------------------------------*/

}} // namespace ntc::detail
