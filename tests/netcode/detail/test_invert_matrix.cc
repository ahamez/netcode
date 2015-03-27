#include "tests/catch.hpp"

#include "netcode/detail/invert_matrix.hh"
#include "netcode/detail/square_matrix.hh"

/*------------------------------------------------------------------------------------------------*/

using namespace ntc;

/*------------------------------------------------------------------------------------------------*/

namespace /* unnamed */{

// Use jerasure implementation as a reference. Slighty modified to use our galois_field.
int
jerasure_invert_matrix( detail::square_matrix& mat, detail::square_matrix& inv
                      , detail::galois_field& gf)
{
  // Types are changed from int to unsigned int to avoid warnings.
  unsigned int i, j, k, x, rs2;
  unsigned int row_start, tmp, inverse;
  unsigned int cols = static_cast<unsigned int>(mat.dimension());
  unsigned int rows = cols;

  k = 0;
  for (i = 0; i < rows; i++) {
    for (j = 0; j < cols; j++) {
      inv[k] = (i == j) ? 1 : 0;
      k++;
    }
  }

  /* First -- convert into upper triangular  */
  for (i = 0u; i < cols; i++) {
    row_start = cols * i;

    /* Swap rows if we ave a zero i,i element.  If we can't swap, then the
     matrix was not invertible  */

    if (mat[row_start+i] == 0) {
      for (j = i+1; j < rows && mat[cols*j+i] == 0; j++) ;
      if (j == rows) return -1;
      rs2 = j*cols;
      for (k = 0; k < cols; k++) {
        tmp = mat[row_start+k];
        mat[row_start+k] = mat[rs2+k];
        mat[rs2+k] = tmp;
        tmp = inv[row_start+k];
        inv[row_start+k] = inv[rs2+k];
        inv[rs2+k] = tmp;
      }
    }

    /* Multiply the row by 1/element i,i  */
    tmp = mat[row_start+i];
    if (tmp != 1) {
//      inverse = galois_single_divide(1, tmp, w);
      inverse = gf.divide(1, tmp);
      for (j = 0; j < cols; j++) {
//        mat[row_start+j] = galois_single_multiply(mat[row_start+j], inverse, w);
        mat[row_start+j] = gf.multiply(mat[row_start+j], inverse);
//        inv[row_start+j] = galois_single_multiply(inv[row_start+j], inverse, w);
        inv[row_start+j] = gf.multiply(inv[row_start+j], inverse);
      }
    }

    /* Now for each j>i, add A_ji*Ai to Aj  */
    k = row_start+i;
    for (j = i+1; j != cols; j++) {
      k += cols;
      if (mat[k] != 0) {
        if (mat[k] == 1) {
          rs2 = cols*j;
          for (x = 0; x < cols; x++) {
            mat[rs2+x] ^= mat[row_start+x];
            inv[rs2+x] ^= inv[row_start+x];
          }
        } else {
          tmp = mat[k];
          rs2 = cols*j;
          for (x = 0; x < cols; x++) {
//            mat[rs2+x] ^= galois_single_multiply(tmp, mat[row_start+x], w);
            mat[rs2+x] ^= gf.multiply(tmp, mat[row_start+x]);
//            inv[rs2+x] ^= galois_single_multiply(tmp, inv[row_start+x], w);
            inv[rs2+x] ^= gf.multiply(tmp, inv[row_start+x]);
          }
        }
      }
    }
  }

  /* Now the matrix is upper triangular.  Start at the top and multiply down  */

//  for (i = rows-1; i >= 0; i--) {
  for (i = rows-1; ; i--) {
    row_start = i*cols;
    for (j = 0; j < i; j++) {
      rs2 = j*cols;
      if (mat[rs2+i] != 0) {
        tmp = mat[rs2+i];
        mat[rs2+i] = 0;
        for (k = 0; k < cols; k++) {
//          inv[rs2+k] ^= galois_single_multiply(tmp, inv[row_start+k], w);
          inv[rs2+k] ^= gf.multiply(tmp, inv[row_start+k]);
        }
      }
    }
    if (i == 0) // Added test .
    {
      break;
    }
  }
  return 0;
}

} // namespace unnamed

/*------------------------------------------------------------------------------------------------*/

TEST_CASE("Compare with jerasure matrix inversion", "[square_matrix][invert]" )
{
  detail::galois_field gf{8};

  detail::square_matrix m0{3};
  std::uint32_t k = 0u;
  for (auto i = 0ul; i < 3; ++i)
  {
    for (auto j = 0ul; j < 3; ++j, ++k)
    {
      m0(i, j) = k;
    }
  }
  auto m1 = m0;

  detail::square_matrix inv0{3};
  detail::square_matrix inv1{3};

  // Matrix is invertible.
  REQUIRE(jerasure_invert_matrix(m0, inv0, gf) == 0);
  REQUIRE(detail::invert(gf, m1, inv1) == std::numeric_limits<std::size_t>::max());

  // The result is the same as jerasure's.
  REQUIRE(inv0.vec() == inv1.vec());
}

/*------------------------------------------------------------------------------------------------*/

TEST_CASE("Non-invertible matrix", "[square_matrix][invert]" )
{
  detail::galois_field gf{8};

  detail::square_matrix m0{3};
  std::uint32_t k = 0u;
  for (auto i = 0u; i < 3; ++i)
  {
    for (auto j = 0u; j < 3; ++j, ++k)
    {
      m0[k] = (k+i) * 2;
    }
  }
  auto m1 = m0;

  detail::square_matrix inv0{3};
  detail::square_matrix inv1{3};

  REQUIRE(jerasure_invert_matrix(m0, inv0, gf) == -1);
  REQUIRE(detail::invert(gf, m1, inv1) != std::numeric_limits<std::size_t>::max());
}

/*------------------------------------------------------------------------------------------------*/

TEST_CASE("Matrix is correctly inverted", "[square_matrix][invert]")
{
  detail::galois_field gf{8};

  detail::square_matrix m{3};
  std::uint32_t k = 0u;
  for (auto i = 0ul; i < 3; ++i)
  {
    for (auto j = 0ul; j < 3; ++j, ++k)
    {
      m(i, j) = k;
    }
  }

  // Inverted matrix is destroyed, we need a copy.
  auto copy = m;

  detail::square_matrix inv{3};

  // Matrix is invertible.
  REQUIRE(detail::invert(gf, copy, inv) == std::numeric_limits<std::size_t>::max());

  // Multiply m and inv
  detail::square_matrix identity{3};
  for (auto i = 0ul; i < identity.dimension(); ++i)
  {
    for (auto j = 0ul; j < identity.dimension(); ++j, ++k)
    {
      identity(i, j) = [&]
      {
        std::uint32_t tmp = 0;
        for (auto k = 0ul; k < identity.dimension(); ++k)
        {
          tmp ^= gf.multiply(m(i, k), inv(k, j));
        }
        return tmp;
      }();
    }
  }

  // Check that M * M^-1 = Id
  for (auto i = 0ul; i < identity.dimension(); ++i)
  {
    for (auto j = 0ul; j < identity.dimension(); ++j)
    {
      REQUIRE(identity(i,j) == (i == j ? 1 : 0));
    }
  }
}

/*------------------------------------------------------------------------------------------------*/
