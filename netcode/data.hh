#pragma once

#include <algorithm> // copy, copy_n
#include <iterator>  // distance

#include "netcode/detail/buffer.hh"

namespace ntc {

/*------------------------------------------------------------------------------------------------*/

/// @brief A data with an allocated buffer.
///
/// @attention When using buffer() to directly write data, because it's not possible for the library
/// to keep track of the number of written bytes, when the data is completely written in the
/// buffer, used_bytes() must be called.
/// This method indicates how many bytes of the allocated buffer are really used. In debug mode, an
/// assertion will be raised if it's not the case.
/// @note It's possible to resize the buffer using resize_buffer().
class data final
{
public:

  /// @brief Constructor.
  /// @param size The size of the buffer to allocate.
  ///
  /// Use this constructor when you intend to directly write in the underlying buffer to avoid copy.
  data(std::uint16_t size)
    : used_bytes_{0}
    , buffer_(size)
  {}

  /// @brief Constructor.
  /// @param src The address of the data to copy.
  /// @param len The size of the data to copy.
  ///
  /// Use this constructor when you need to copy the input data.
  data(const char* src, std::uint16_t len)
    : used_bytes_{len}
    , buffer_(len)
  {
    std::copy_n(src, len, buffer_.begin());
  }

  /// @brief Constructor.
  /// @param begin The beginning of the data to copy.
  /// @param end The end of the data to copy.
  ///
  /// Use this constructor when you need to copy the input data.
  template <typename InputIterator>
  data(InputIterator begin, InputIterator end)
    : used_bytes_{static_cast<std::uint16_t>(std::distance(begin, end))}
    , buffer_(used_bytes_)
  {
    std::copy(begin, end, buffer_.begin());
  }

  /// @brief Get the buffer where to write the data.
  char*
  buffer()
  noexcept
  {
    return buffer_.data();
  }

  /// @brief Resize the buffer.
  /// @param size The new buffer size.
  ///
  /// May cause a copy.
  void
  resize(std::uint16_t size)
  {
    buffer_.resize(size);
  }

  /// @brief Get the the number of used bytes for this data (read-only).
  std::uint16_t
  used_bytes()
  const noexcept
  {
    return used_bytes_;
  }

  /// @brief Get the the number of used bytes for this data.
  ///
  /// When set, it must be smaller than the reserved size.
  std::uint16_t&
  used_bytes()
  noexcept
  {
    return used_bytes_;
  }

  /// @brief Get the implementation-defined underlying buffer (read-only).
  const detail::byte_buffer&
  buffer_impl()
  const noexcept
  {
    return buffer_;
  }

  /// @brief Get the implementation-defined underlying buffer.
  detail::byte_buffer&
  buffer_impl()
  noexcept
  {
    return buffer_;
  }

  /// @brief The useable size of the underlying buffer.
  /// @note It might be greater than the requested size at construction or by resize().
  std::uint16_t
  reserved_size()
  const noexcept
  {
    return static_cast<std::uint16_t>(buffer_.size());
  }

  /// @brief Reset the data for further re-use.
  /// @note Can be used on a moved data. As a matter of fact, it's the only way to use again a
  /// moved data.
  void
  reset(std::uint16_t size)
  noexcept
  {
    used_bytes_ = 0;
    buffer_.resize(size);
  }

private:

  /// @brief The size of the data given by the user.
  std::uint16_t used_bytes_;

  /// @brief The buffer storage.
  detail::byte_buffer buffer_;
};

/*------------------------------------------------------------------------------------------------*/

} // namespace ntc
