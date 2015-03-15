#pragma once

#include <algorithm> // copy_n
#include <iterator>  // back_inserter

#include "netcode/detail/multiple.hh"
#include "netcode/detail/symbol_buffer.hh"

namespace ntc { namespace detail {

/*------------------------------------------------------------------------------------------------*/

/// @internal
/// @brief The base class for symbol, not intended for direct usage.
class symbol_base
{
public:

  /// @brief Get the real size of the buffer.
  ///
  /// Due to internal alignement considerations, it might be larger than the user size.
  std::size_t
  buffer_size()
  const noexcept
  {
    return buffer_.size();
  }

  /// @brief The size of the symbol given by the user, (the number of bytes written by the user).
  std::size_t
  user_size()
  const noexcept
  {
    return user_size_;
  }

  /// @internal
  detail::symbol_buffer&
  buffer()
  noexcept
  {
    return buffer_;
  }

protected:

  /// @brief Tag to discriminate the constructor which reserves a size.
  struct reserve_only{};

  /// @brief Initialize the buffer.
  /// @param buffer_size The buffer size.
  symbol_base(std::size_t buffer_size)
    : buffer_(detail::make_multiple(buffer_size, 16))
    , user_size_{0}
  {}

  /// @brief Reserve memory for the buffer.
  /// @param reserve_size The size to reserve.
  symbol_base(std::size_t reserve_size, reserve_only)
    : buffer_()
    , user_size_{0}
  {
    buffer_.reserve(detail::make_multiple(reserve_size, 16));
  }

  /// @brief Initialize the buffer with a copy of a memory location.
  /// @param len The size of the data.
  /// @param src The data to copy.
  symbol_base(std::size_t len, const char* src)
    : buffer_(detail::make_multiple(len, 16))
    , user_size_{len}
  {
    std::copy_n(src, len, buffer_.begin());
  }

  /// @brief Can't delete through this base class.
  ~symbol_base(){}

  /// @brief The size of the symbol given by the user.
  std::size_t user_size_;

private:

  /// @brief The buffer storage.
  detail::symbol_buffer buffer_;
};

/*------------------------------------------------------------------------------------------------*/

}} // namespace ntc::detail
