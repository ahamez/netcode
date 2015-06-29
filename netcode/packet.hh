#pragma once

#include <algorithm> // copy
#include <initializer_list>

#include "netcode/detail/buffer.hh"
#include "netcode/detail/symbol_alignment.hh"

namespace ntc {

/*------------------------------------------------------------------------------------------------*/

/// @brief The type to hold data coming from the network
///
/// It's a wrapper around a std::vector<char>. Most of this container's member functions are
/// replicated, thus the STL documentation is sufficient for this type.
class packet
{
public:

  /// @internal
  /// @brief The number of bytes before the symbol
  static constexpr auto alignment = detail::symbol_alignment;

  /// @internal
  /// @brief The number of bytes before the beginning of the packet
  static constexpr auto shift = detail::symbol_alignment - detail::source_and_repair_headers;

public:

  using value_type = detail::byte_buffer::value_type;
  using size_type = detail::byte_buffer::size_type;
  using difference_type = detail::byte_buffer::size_type;
  using pointer = detail::byte_buffer::pointer;
  using const_pointer = detail::byte_buffer::const_pointer;
  using reference = detail::byte_buffer::reference;
  using const_reference = detail::byte_buffer::const_reference;
  using iterator = detail::byte_buffer::iterator;
  using const_iterator = detail::byte_buffer::const_iterator;

public:

  packet(const packet&) = default;
  packet& operator=(const packet&) = default;
  packet(packet&&) = default;
  packet& operator=(packet&&) = default;

  packet()
    : m_buffer(shift)
  {}

  explicit packet(size_type size)
    : m_buffer(size + shift)
  {}

  packet(size_type size, char value)
    : m_buffer(size + shift, value)
  {}

  template <typename InputIterator>
  packet(InputIterator first, InputIterator last)
    : m_buffer(static_cast<size_type>(std::distance(first, last)) + shift)
  {
    std::copy(first, last, m_buffer.begin() + shift);
  }

  packet(std::initializer_list<char> init)
    : m_buffer(init.size() + shift)
  {
    std::copy(std::begin(init), std::end(init), m_buffer.begin() + shift);
  }

  /// @internal
  /// @note For testing purposes only
  packet(const detail::byte_buffer& symbol)
    : m_buffer(symbol.size() + alignment)
  {
    std::copy(symbol.begin(), symbol.end(), m_buffer.begin() + alignment);
  }

  /// @internal
  /// @note For testing purposes only
  packet(const detail::zero_byte_buffer& symbol)
    : m_buffer(symbol.size() + alignment)
  {
    std::copy(symbol.begin(), symbol.end(), m_buffer.begin() + alignment);
  }

  void
  assign(size_type count, char value)
  {
    resize(count);
    std::fill(begin(), end(), value);
  }

  template <typename InputIterator>
  void
  assign(InputIterator first, InputIterator last)
  {
    resize(static_cast<size_type>(std::distance(first, last)));
    std::copy(first, last, begin());
  }

  reference
  operator[](size_type pos)
  noexcept
  {
    return m_buffer[pos + shift];
  }

  const_reference
  operator[](size_type pos)
  const noexcept
  {
    return m_buffer[pos + shift];
  }

  char*
  data()
  noexcept
  {
    return m_buffer.data() + shift;
  }

  const char*
  data()
  const noexcept
  {
    return m_buffer.data() + shift;
  }

  iterator
  begin()
  noexcept
  {
    return m_buffer.begin() + shift;
  }

  const_iterator
  begin()
  const noexcept
  {
    return m_buffer.begin() + shift;
  }

  const_iterator
  cbegin()
  const noexcept
  {
    return m_buffer.cbegin() + shift;
  }

  iterator
  end()
  noexcept
  {
    return m_buffer.end();
  }

  const_iterator
  end()
  const noexcept
  {
    return m_buffer.end();
  }

  const_iterator
  cend()
  const noexcept
  {
    return m_buffer.cend();
  }

  bool
  empty()
  const noexcept
  {
    return size() == 0;
  }

  size_type
  size()
  const noexcept
  {
    return m_buffer.size() - shift;
  }

  void
  reserve(size_type new_cap)
  {
    m_buffer.reserve(new_cap + shift);
  }

  size_type
  capacity()
  const noexcept
  {
    return m_buffer.capacity() - shift;
  }

  void
  shrink_to_fit()
  {
    m_buffer.shrink_to_fit();
  }

  void
  clear()
  {
    m_buffer.resize(shift);
  }

  void
  push_back(char value)
  {
    m_buffer.push_back(value);
  }

  void
  resize(size_type size)
  {
    m_buffer.resize(size + shift);
  }

  void
  resize(size_type size, char value)
  {
    m_buffer.resize(size + shift, value);
  }

  /// @internal
  const char*
  symbol()
  const noexcept
  {
    return m_buffer.data() + alignment;
  }

  /// @internal
  char*
  symbol()
  noexcept
  {
    return m_buffer.data() + alignment;
  }

private:

  detail::byte_buffer m_buffer;
};

/*------------------------------------------------------------------------------------------------*/

} // namespace ntc
