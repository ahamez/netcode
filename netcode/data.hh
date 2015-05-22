#pragma once

#include <algorithm> // copy, copy_n
#include <iterator>  // distance

#include "netcode/detail/buffer.hh"

namespace ntc {

/*------------------------------------------------------------------------------------------------*/

/// @brief Class to store data given to encoder.
///
/// @attention When using buffer() to directly write data, because it's not possible for the library
/// to keep track of the number of written bytes, when the data is completely written in the
/// buffer, used_bytes() must be called.
/// This method indicates how many bytes of the allocated buffer are really used. In debug mode, an
/// assertion will be raised if it's not the case.
/// @note It's possible to resize the buffer using resize_buffer().
/// @ingroup netcode
class data final
{
public:

  /// @brief Constructor.
  /// @param size The size of the buffer to allocate.
  ///
  /// Use this constructor when you intend to directly write in the underlying buffer to avoid copy.
  explicit data(std::uint16_t size)
    : m_used_bytes{0}
    , m_buffer(size)
  {}

  /// @brief Constructor.
  /// @param src The address of the data to copy.
  /// @param len The size of the data to copy.
  ///
  /// Use this constructor when you need to copy the input data.
  data(const char* src, std::uint16_t len)
    : m_used_bytes{len}
    , m_buffer(len)
  {
    std::copy_n(src, len, m_buffer.begin());
  }

  /// @brief Constructor.
  /// @param begin The beginning of the data to copy.
  /// @param end The end of the data to copy.
  ///
  /// Use this constructor when you need to copy the input data.
  template <typename InputIterator>
  data(InputIterator begin, InputIterator end)
    : m_used_bytes{static_cast<std::uint16_t>(std::distance(begin, end))}
    , m_buffer(m_used_bytes)
  {
    std::copy(begin, end, m_buffer.begin());
  }

  /// @brief Get the buffer where to write the data.
  char*
  buffer()
  noexcept
  {
    return m_buffer.data();
  }

  /// @brief Resize the buffer.
  /// @param size The new buffer size.
  ///
  /// May cause a copy.
  void
  resize(std::uint16_t size)
  {
    m_buffer.resize(size);
  }

  /// @brief Get the the number of used bytes for this data (read-only).
  std::uint16_t
  used_bytes()
  const noexcept
  {
    return m_used_bytes;
  }

  /// @brief Get the the number of used bytes for this data.
  ///
  /// When set, it must be smaller than the reserved size.
  std::uint16_t&
  used_bytes()
  noexcept
  {
    return m_used_bytes;
  }

  /// @brief Get the implementation-defined underlying buffer (read-only).
  const detail::byte_buffer&
  buffer_impl()
  const noexcept
  {
    return m_buffer;
  }

  /// @brief Get the implementation-defined underlying buffer.
  detail::byte_buffer&
  buffer_impl()
  noexcept
  {
    return m_buffer;
  }

  /// @brief The useable size of the underlying buffer.
  /// @note It might be greater than the requested size at construction or by resize().
  std::uint16_t
  reserved_size()
  const noexcept
  {
    return static_cast<std::uint16_t>(m_buffer.size());
  }

  /// @brief Reset the data for further re-use.
  /// @note Can be used on a moved data. As a matter of fact, it's the only way to use again a
  /// moved data.
  void
  reset(std::uint16_t size)
  {
    m_used_bytes = 0;
    m_buffer.resize(size);
  }

private:

  /// @brief The size of the data given by the user.
  std::uint16_t m_used_bytes;

  /// @brief The buffer storage.
  detail::byte_buffer m_buffer;
};

/*------------------------------------------------------------------------------------------------*/

} // namespace ntc
