#pragma once

#include <algorithm> // copy, copy_n

#include "netcode/detail/buffer.hh"
#include "netcode/detail/multiple.hh"

namespace ntc {

/*------------------------------------------------------------------------------------------------*/

/// @brief A packet with an allocated buffer.
/// @note It's possible to resize the buffer using resize_buffer().
///
/// This class is meant to be used with encoder::notify() and decoder::notify().
class packet final
{
public:

  /// @brief Constructor.
  /// @param size The size of the buffer to allocate.
  packet(std::size_t size)
    : buffer_(detail::make_multiple(size, 16))
  {}

  /// @brief Get the buffer where to write the packet.
  char*
  buffer()
  noexcept
  {
    return buffer_.data();
  }

  /// @brief Get the buffer (read-only).
  const char*
  buffer()
  const noexcept
  {
    return buffer_.data();
  }

  /// @brief Resize the buffer.
  /// @param size The new buffer size.
  ///
  /// May cause a copy.
  void
  resize(std::size_t size)
  {
    buffer_.resize(size);
  }

  /// @brief The useable size of the underlying buffer.
  /// @note It might be greater than the requested size at construction.
  std::size_t
  reserved_size()
  const noexcept
  {
    return buffer_.size();
  }

private:

  /// @brief The buffer storage.
  detail::byte_buffer buffer_;
};

/*------------------------------------------------------------------------------------------------*/

/// @brief A packet that copies the input data.
///
/// This class is meant to be used with encoder::notify() and decoder::notify().
/// Use this packet when the data already exists and must be copied, otherwise @ref auto_packet and
/// @ref packet should be prefered.
class copy_packet final
{
public:

  /// @brief Constructor.
  /// @param src The address of the data to copy.
  /// @param len The size of the data to copy.
  copy_packet(const char* src, std::size_t len)
    : buffer_(detail::make_multiple(len, 16))
  {
    std::copy_n(src, len, buffer_.begin());
  }

  /// @brief Constructor.
  /// @param begin The beginning of the data to copy.
  /// @param end The end of the data to copy.
  template <typename InputIterator>
  copy_packet(InputIterator begin, InputIterator end)
    : buffer_(begin, end)
  {}

  /// @brief Get the buffer where to write the packet.
  char*
  buffer()
  noexcept
  {
    return buffer_.data();
  }

  /// @brief Get the buffer (read-only).
  const char*
  buffer()
  const noexcept
  {
    return buffer_.data();
  }

  /// @brief The useable size of the underlying buffer.
  /// @note It might be greater than the requested size at construction.
  std::size_t
  reserved_size()
  const noexcept
  {
    return buffer_.size();
  }

private:

  /// @brief The buffer storage.
  detail::byte_buffer buffer_;
};

/*------------------------------------------------------------------------------------------------*/

} // namespace ntc
