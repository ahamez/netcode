#pragma once

#include "netcode/detail/buffer.hh"

namespace ntc { namespace detail {

/*------------------------------------------------------------------------------------------------*/

/// @internal
class square_matrix final
{
public:

  square_matrix(std::size_t n)
    : n_{n}
    , vec_(n*n)
  {}

  std::uint32_t
  operator()(std::size_t row, std::size_t col)
  const noexcept
  {
    return vec_[col * n_ + row];
  }

  std::uint32_t&
  operator()(std::size_t row, std::size_t col)
  noexcept
  {
     return vec_[col * n_ + row];
  }

  const std::uint32_t*
  column(std::size_t col)
  const noexcept
  {
    return vec_.data() + (col * n_);
  }

  std::uint32_t*
  column(std::size_t col)
  noexcept
  {
    return vec_.data() + (col * n_);
  }

  std::uint32_t
  operator[](std::size_t index)
  const noexcept
  {
    return vec_[index];
  }

  std::uint32_t&
  operator[](std::size_t index)
  noexcept
  {
    return vec_[index];
  }

  void
  resize(std::size_t n)
  {
    vec_.resize(n*n);
    n_ = n;
  }

  std::size_t
  dimension()
  const noexcept
  {
    return n_;
  }

  const buffer<std::uint32_t>&
  vec()
  const noexcept
  {
    return vec_;
  }

private:

  std::size_t n_;
  buffer<std::uint32_t> vec_;
};

/*------------------------------------------------------------------------------------------------*/

}} // namespace ntc::detail
