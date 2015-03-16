#pragma once

#include <vector>

#include <boost/align/aligned_allocator_adaptor.hpp>

namespace ntc { namespace detail {

/*------------------------------------------------------------------------------------------------*/

/// @brief Allocator to avoid value initialization.
/// @see http://stackoverflow.com/a/21028912/21584
template <typename T, typename Alloc = std::allocator<T>>
class default_init_allocator
  : public Alloc
{
private:

  using a_t = std::allocator_traits<Alloc>;

public:

  /// @brief Constructor. Forward to base clase constructor.
  ///
  /// @note @code using Alloc::Alloc @endcode would be the right way to write it,
  /// but GCC 4.7 doesn't recognize this feature.
  template <typename... Args>
  default_init_allocator(Args&&... args)
    : Alloc{std::forward<Args>(args)...}
  {}

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
    a_t::construct(static_cast<Alloc&>(*this), ptr, std::forward<Args>(args)...);
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
