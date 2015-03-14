#pragma once

#include <vector>

#include <boost/align/aligned_allocator_adaptor.hpp>

namespace ntc { namespace detail {

/*------------------------------------------------------------------------------------------------*/

/// @brief Allocator to avoid value initialization.
/// @see http://stackoverflow.com/a/21028912/21584
template <typename T, typename A = std::allocator<T>>
class default_init_allocator
  : public A
{
private:

  using a_t = std::allocator_traits<A>;

public:

  using A::A;

  template <typename U>
  struct rebind
  {
    using other = default_init_allocator<U, typename a_t::template rebind_alloc<U>>;
  };

  template <typename U>
  void
  construct(U* ptr)
  {
    ::new (static_cast<void*>(ptr)) U;
  }

  template <typename U, typename...Args>
  void
  construct(U* ptr, Args&&... args)
  {
    a_t::construct(static_cast<A&>(*this), ptr, std::forward<Args>(args)...);
  }
};

/*------------------------------------------------------------------------------------------------*/

/// @brief An aligned allocator that aligns and avoid value initialization.
template <typename T, std::size_t Align>
using default_init_aligned_alloc
  = boost::alignment::aligned_allocator_adaptor<default_init_allocator<T>, Align>;

/*------------------------------------------------------------------------------------------------*/

/// @brief An aligned buffer of bytes.
using symbol_buffer = std::vector<char, default_init_aligned_alloc<char, 16>>;

/*------------------------------------------------------------------------------------------------*/

}} // namespace ntc::detail
