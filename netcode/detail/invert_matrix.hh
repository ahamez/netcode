#pragma once

#include <cassert>
#include <limits> // numeric_limits

#include "netcode/detail/galois_field.hh"
#include "netcode/detail/square_matrix.hh"

namespace ntc { namespace detail {

/*------------------------------------------------------------------------------------------------*/

/// @internal
/// @brief Invert a matrix using a Galois fiels.
/// @attention @p mat will be overwritten
/// @note This is the algorithm provided by jerasure ( http://jerasure.org )
/// @related square_matrix
inline
std::size_t
invert(square_matrix& mat, square_matrix& inv, galois_field& gf)
noexcept
{
  assert(mat.dimension() == inv.dimension());

  const auto cols = mat.dimension();
  const auto rows = mat.dimension();

  for (auto i = 0ul; i < rows; ++i)
  {
    for (auto j = 0ul; j < cols; ++j)
    {
      inv(i,j) = (i == j) ? 1 : 0;
    }
  }

  // Convert into upper triangular
  for (auto i = 0ul; i < cols; ++i)
  {
    const auto row_start = i * cols;
    const auto row = i;

    // Swap rows if we have a zero i,i element.
    // If we can't swap, then the matrix was not invertible.
    if (mat[row_start + i] == 0)
    {
      auto j = 0ul;
      for (j = i + 1; j < rows and mat[(cols * j) + i] == 0; ++j)
      {
      }

      if (j == rows)
      {
        // Failure, matrix is not invertible.
        return (j - 1);
      }

      const auto row_start2 = j * cols;
      for (auto k = 0ul; k < cols; ++k)
      {
        auto tmp = mat[row_start + k];
        mat[row_start + k] = mat[row_start2 + k];
        mat[row_start2 + k] = tmp;

        tmp = inv[row_start + k];
        inv[row_start + k] = inv[row_start2 + k];
        inv[row_start2 + k] = tmp;
      }
    }
 
    // Multiply the row by 1/element i,i
    const auto tmp = mat[row_start + i];
    if (tmp != 1)
    {
      const auto inverse = gf.divide(1, tmp);
      for (auto j = 0ul; j < cols; j++)
      {
        mat[row_start + j] = gf.multiply(mat[row_start + j], inverse);
        inv[row_start + j] = gf.multiply(inv[row_start + j], inverse);
      }
    }

    // Now for each j > i, add A_ji * Ai to Aj
    auto k = row_start + i;
    for (auto j = i + 1; j != cols; ++j)
    {
      k += cols;
      if (mat[k] != 0)
      {
        if (mat[k] == 1)
        {
          const auto row_start2 = cols * j;
          for (auto x = 0ul; x < cols; ++x)
          {
            mat[row_start2 + x] ^= mat[row_start + x];
            inv[row_start2 + x] ^= inv[row_start + x];
          }
        }
        else
        {
          const auto tmp = mat[k];
          const auto rs2 = cols * j;
          for (auto x = 0ul; x < cols; ++x)
          {
            mat[rs2 + x] ^= gf.multiply(tmp, mat[row_start + x]);
            inv[rs2 + x] ^= gf.multiply(tmp, inv[row_start + x]);
          }
        }
      }
    }
  }

  // Now the matrix is upper triangular. Start at the top and multiply down.
  for (auto i = rows - 1; ; --i)
  {
    auto row_start = i * cols;
    for (auto j = 0ul; j < i; ++j)
    {
      const auto rs2 = j * cols;
      if (mat[rs2+i] != 0)
      {
        const auto tmp = mat[rs2+i];
        mat[rs2 + i] = 0;
        for (auto k = 0ul; k < cols; ++k)
        {
          inv[rs2 + k] ^= gf.multiply(tmp, inv[row_start + k]);
        }
      }
    }

    if (i == 0)
    {
      break;
    }
  }

  // Everything went OK.
  return std::numeric_limits<std::size_t>::max();
}

/*------------------------------------------------------------------------------------------------*/

}} // namespace ntc::detail
