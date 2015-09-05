#pragma once

#include "netcode/detail/buffer.hh"

namespace ntc { namespace detail {

/*------------------------------------------------------------------------------------------------*/

/// @internal
/// @brief A resizable square matrix suitable for value on 32 bits, stored in contiguous memory
/// @note The matrix is not initialized to 0, even when constructed or resized
class square_matrix final
{
public:

  /// @brief Construct a @p n * @p n square matrix
  explicit square_matrix(std::size_t n)
    : m_dimension{n}
    , m_vec(n*n)
  {}

  std::uint32_t
  operator()(std::size_t row, std::size_t col)
  const noexcept
  {
    return m_vec[col * m_dimension + row];
  }

  std::uint32_t&
  operator()(std::size_t row, std::size_t col)
  noexcept
  {
     return m_vec[col * m_dimension + row];
  }

  const std::uint32_t*
  column(std::size_t col)
  const noexcept
  {
    return m_vec.data() + (col * m_dimension);
  }

  std::uint32_t*
  column(std::size_t col)
  noexcept
  {
    return m_vec.data() + (col * m_dimension);
  }

  std::uint32_t
  operator[](std::size_t index)
  const noexcept
  {
    return m_vec[index];
  }

  std::uint32_t&
  operator[](std::size_t index)
  noexcept
  {
    return m_vec[index];
  }

  void
  resize(std::size_t n)
  {
    m_vec.resize(n*n);
    m_dimension = n;
  }

  std::size_t
  dimension()
  const noexcept
  {
    return m_dimension;
  }

private:

  std::size_t m_dimension;
  buffer<std::uint32_t> m_vec;
};

/*------------------------------------------------------------------------------------------------*/

}} // namespace ntc::detail
