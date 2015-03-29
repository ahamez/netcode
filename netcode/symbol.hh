#pragma once

#include <algorithm> // copy, copy_n
#include <iterator>  // back_inserter, distance

#include "netcode/detail/buffer.hh"
#include "netcode/detail/multiple.hh"

namespace ntc {

/*------------------------------------------------------------------------------------------------*/

/// @brief A symbol with an allocated buffer.
///
/// @attention Because it's not possible for the library to keep track of the number of written
/// bytes, when the symbol is completely written in the buffer, set_user_size() must be called.
/// This method indicates how many bytes of the allocated buffer are really used. In debug mode, an
/// assertion will be raised if it's not the case.
/// @note It's possible to resize the buffer using resize_buffer().
/// This class is meant to be used with encoder::commit().
class symbol final
{
public:

  /// @brief Constructor.
  /// @param size The size of the buffer to allocate.
  symbol(std::size_t size)
    : user_size_{0}
    , buffer_(detail::make_multiple(size, 16))
  {}

  /// @brief Get the buffer where to write the symbol.
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
  resize_buffer(std::size_t size)
  {
    buffer_.resize(size);
  }

  /// @brief Tell the library how many bytes were written in the buffer.
  void
  set_nb_written_bytes(std::size_t sz)
  noexcept
  {
    user_size_ = sz;
  }

private:

  /// @brief The size of the symbol given by the user.
  std::size_t user_size_;

  /// @brief The buffer storage.
  detail::byte_buffer buffer_;

  /// @brief The encoder needs to set the user size and to access the buffer.
  friend class encoder;
};

/*------------------------------------------------------------------------------------------------*/

/// @brief A symbol which automatically grows as needed.
///
/// This class is meant to be used with encoder::commit().
/// It provides the easiest way to avoid copying, and can be used with STL algorithms.
class auto_symbol final
{
public:

  /// @brief Construct with a reserved buffer to avoid memory re-allocations when growing.
  /// @param reserve_size The size to reserve.
  auto_symbol(std::size_t reserve_size)
    : user_size_{0}
    , buffer_()
  {
    buffer_.reserve(reserve_size);
  }

  /// @brief Default constructor with a default size (512 bytes) for the reserved buffer.
  auto_symbol()
    : auto_symbol{512}
  {}

  /// @brief An iterator to write in the symbol buffer.
  using back_insert_iterator = std::back_insert_iterator<detail::byte_buffer>;

  /// @brief Get a back inserter iterator to the symbol buffer.
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
  /// @note The reserved memory is kept.
  void
  reset()
  noexcept
  {
    buffer_.clear();
  }

private:

  /// @brief The size of the symbol given by the user.
  ///
  /// Will be set by encoder::commit().
  std::size_t user_size_;

  /// @brief The buffer storage.
  detail::byte_buffer buffer_;

  /// @brief The encoder needs to set the user size and to access the buffer.
  friend class encoder;
};

/*------------------------------------------------------------------------------------------------*/

/// @brief A symbol that copies the input data.
///
/// This class is meant to be used with encoder::commit().
/// Use this symbol when the data already exists and must be copied, otherwise @ref auto_symbol and
/// @ref symbol should be prefered.
class copy_symbol final
{
public:

  /// @brief Constructor.
  /// @param src The address of the data to copy.
  /// @param len The size of the data to copy.
  copy_symbol(const char* src, std::size_t len)
    : user_size_{len}
    , buffer_(detail::make_multiple(len, 16))
  {
    std::copy_n(src, len, buffer_.begin());
  }

  /// @brief Constructor.
  /// @param begin The beginning of the data to copy.
  /// @param end The end of the data to copy.
  template <typename InputIterator>
  copy_symbol(InputIterator begin, InputIterator end)
    : user_size_{static_cast<std::size_t>(std::distance(begin, end))}
    , buffer_(detail::make_multiple(user_size_, 16))
  {
    std::copy(begin, end, buffer_.begin());
  }

private:

  /// @brief The size of the symbol given by the user.
  ///
  /// Will be set by encoder::commit().
  std::size_t user_size_;

  /// @brief The buffer storage.
  detail::byte_buffer buffer_;

  /// @brief The encoder needs to access the buffer.
  friend class encoder;
};

/*------------------------------------------------------------------------------------------------*/

} // namespace ntc
