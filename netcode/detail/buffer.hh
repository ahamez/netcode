#pragma once

#include <memory>
#include <utility>
#include <vector>

#include <boost/align/aligned_allocator.hpp>
#include <boost/align/aligned_allocator_adaptor.hpp>

namespace ntc { namespace detail {

/*------------------------------------------------------------------------------------------------*/

/// @internal
/// @brief Allocator to avoid value initialization
/// @see http://stackoverflow.com/a/21028912/21584
template <typename T, typename Alloc = std::allocator<T>>
class default_init_allocator : public Alloc
{
private:
  using base = Alloc;
  using traits = std::allocator_traits<Alloc>;

public:
  using base::base;

  template <typename U>
  struct rebind
  {
    using other = default_init_allocator<U, typename traits::template rebind_alloc<U>>;
  };

  template <typename U>
  void
  construct(U* ptr)
  {
    std::construct_at(ptr);
  }

  template <typename U, typename...Args>
  void
  construct(U* ptr, Args&&... args)
  {
    traits::construct(static_cast<base&>(*this), ptr, std::forward<Args>(args)...);
  }
};

/*------------------------------------------------------------------------------------------------*/

/// @internal
/// @brief An aligned allocator that aligns and avoid value initialization
template <typename T, std::size_t Align>
using default_init_aligned_alloc
  = boost::alignment::aligned_allocator_adaptor<default_init_allocator<T>, Align>;

/*------------------------------------------------------------------------------------------------*/

/// @internal
/// @brief A generic buffer aligned on 16 bytes
template <typename T>
using buffer = std::vector<T, default_init_aligned_alloc<T, 16>>;

/*------------------------------------------------------------------------------------------------*/

/// @internal
/// @brief An buffer of bytes aligned on 16 bytes
using byte_buffer = buffer<char>;

/*------------------------------------------------------------------------------------------------*/

/// @internal
/// @brief An buffer of bytes aligned on 16 bytes
/// @note Will set new bytes to 0 when resized
///
/// Use when a buffer with a default initialization is required.
using zero_byte_buffer = std::vector<char, boost::alignment::aligned_allocator<char, 16>>;

/*------------------------------------------------------------------------------------------------*/

}} // namespace ntc::detail
