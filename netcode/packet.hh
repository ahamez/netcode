#pragma once

#include <algorithm> // copy_n
#include <iterator>  // back_inserter

#include "netcode/detail/multiple.hh"
#include "netcode/detail/buffer.hh"

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
  resize_buffer(std::size_t size)
  {
    buffer_.resize(size);
  }

private:

  /// @brief The buffer storage.
  detail::buffer_t buffer_;
};

/*------------------------------------------------------------------------------------------------*/

/// @brief A packet which automatically grows as needed.
///
/// This class is meant to be used with encoder::notify() and decoder::notify().
/// It provides the easiest way to avoid copying, and can be used with STL algorithms.
class auto_packet final
{
public:

  /// @brief Construct with a reserved buffer to avoid memory re-allocations when growing.
  /// @param reserve_size The size to reserve.
  auto_packet(std::size_t reserve_size)
    : buffer_()
  {
    buffer_.reserve(reserve_size);
  }

  /// @brief Default constructor with a default size (512 bytes) for the reserved buffer.
  auto_packet()
    : auto_packet{512}
  {}

  /// @brief An iterator to write in the packet buffer.
  using back_insert_iterator = std::back_insert_iterator<detail::buffer_t>;

  /// @brief Get a back inserter iterator to the packet buffer.
  ///
  /// The returned iterator is usable as an output iterator with any STL algorithm.
  /// @note Inserting a number of times greater than the reserved size will result in a new
  /// allocation, and possibly a copy. Thus, the reserved size given at construction should be
  /// large enough to avoid this situation.
  back_insert_iterator
  back_inserter()
  {
    return std::back_inserter(buffer_);
  }

  /// @brief Reset the buffer.
  /// @attention Any current back insert iterator will be invalidated. A new one must be created
  /// with back_inserter().
  ///
  /// The reserved memory is kept.
  void
  reset()
  noexcept
  {
    buffer_.resize(0);
  }

private:

  /// @brief The buffer storage.
  detail::buffer_t buffer_;

  /// @brief The encoder needs to access the buffer.
  friend class encoder;

  /// @brief The decoder needs to access the buffer.
  friend class decoder;
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
  /// @param len The size of the data to copy.
  /// @param src The address of the data to copy.
  copy_packet(std::size_t len, const char* src)
    : buffer_(detail::make_multiple(len, 16))
  {
    std::copy_n(src, len, buffer_.begin());
  }

private:

  /// @brief The buffer storage.
  detail::buffer_t buffer_;

  /// @brief The encoder needs to access the buffer.
  friend class encoder;

  /// @brief The decoder needs to access the buffer.
  friend class decoder;
};

/*------------------------------------------------------------------------------------------------*/

} // namespace ntc
