#pragma once

#include <algorithm> // copy
#include <initializer_list>

#ifdef NTC_DUMP_PACKETS
#include <iostream>
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wconversion"
#include <boost/endian/conversion.hpp>
#pragma GCC diagnostic pop
#endif

#include "netcode/detail/buffer.hh"
#include "netcode/detail/symbol_alignment.hh"

namespace ntc {

/*------------------------------------------------------------------------------------------------*/

/// @ingroup ntc_packets
/// @brief The type to hold data coming from the network
/// @note It's useable as an Asio buffer with the following code:
/// @code
/// namespace asio {
///  inline mutable_buffers_1 buffer(ntc::packet& p)
///  {
///    return mutable_buffers_1{mutable_buffer{p.size() ? p.data() : nullptr, p.size()}};
///  }
///}
/// @endcode
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

  /// @brief Copy constructor
  packet(const packet&) = default;

  /// @brief Copy assignement operator
  packet& operator=(const packet&) = default;

  /// @brief Move constructor
  packet(packet&&) = default;

  /// @brief Move assignement operator
  packet& operator=(packet&&) = default;

  /// @brief Default constructor; constructs an empty packet
  packet()
    : m_buffer(shift)
  {}

  /// @brief Constructs a packet of size @p size; the packet is uninitialized
  explicit packet(size_type size)
    : m_buffer(size + shift)
  {}

  /// @brief Constructs the packet with @p count copies of @p value
  packet(size_type count, char value)
    : m_buffer(count + shift, value)
  {}

  /// @brief Constructs the packet with the contents of the range [@p first, @p last)
  template <typename InputIterator>
  packet(InputIterator first, InputIterator last)
    : m_buffer(static_cast<size_type>(std::distance(first, last)) + shift)
  {
    std::copy(first, last, m_buffer.begin() + shift);
  }

  /// @brief Constructs the packet with an initializer list
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

  /// @brief Replaces the contents with @p count copies of byte @p value
  void
  assign(size_type count, char value)
  {
    resize(count);
    std::fill(begin(), end(), value);
  }

  /// @brief Replaces the contents with the range [@p first, @p last)
  template <typename InputIterator>
  void
  assign(InputIterator first, InputIterator last)
  {
    resize(static_cast<size_type>(std::distance(first, last)));
    std::copy(first, last, begin());
  }

  /// @brief Replaces the contents with the bytes from the initializer list @p init
  void
  assign(std::initializer_list<char> init)
  {
    assign(std::begin(init), std::end(init));
  }

  /// @brief Returns a reference to the element at specified location @p pos; no bounds checking is
  /// performed
  reference
  operator[](size_type pos)
  noexcept
  {
    return m_buffer[pos + shift];
  }

  /// @brief Returns a reference to the element at specified location @p pos; no bounds checking is
  /// performed
  const_reference
  operator[](size_type pos)
  const noexcept
  {
    return m_buffer[pos + shift];
  }

  /// @brief Returns a pointer to the underlying array serving as element storage
  /// @note The pointer is such that range [data(); data() + size()) is always a valid range, even
  /// if the container is empty.
  char*
  data()
  noexcept
  {
    return m_buffer.data() + shift;
  }

  /// @brief Returns a pointer to the underlying array serving as element storage
  /// @note The pointer is such that range [data(); data() + size()) is always a valid range, even
  /// if the container is empty.
  const char*
  data()
  const noexcept
  {
    return m_buffer.data() + shift;
  }

  /// @brief Returns an iterator to the first element of the packet
  /// @note If the container is empty, the returned iterator will be equal to end()
  iterator
  begin()
  noexcept
  {
    return m_buffer.begin() + shift;
  }

  /// @brief Returns an iterator to the first element of the packet
  /// @note If the container is empty, the returned iterator will be equal to end()
  const_iterator
  begin()
  const noexcept
  {
    return m_buffer.begin() + shift;
  }

  /// @brief Returns an iterator to the first element of the packet
  /// @note If the container is empty, the returned iterator will be equal to end()
  const_iterator
  cbegin()
  const noexcept
  {
    return m_buffer.cbegin() + shift;
  }

  /// @brief Returns an iterator to the element following the last element of the packet
  /// @note This element acts as a placeholder; attempting to access it results in undefined
  /// behavior
  iterator
  end()
  noexcept
  {
    return m_buffer.end();
  }

  /// @brief Returns an iterator to the element following the last element of the packet
  /// @note This element acts as a placeholder; attempting to access it results in undefined
  /// behavior
  const_iterator
  end()
  const noexcept
  {
    return m_buffer.end();
  }

  /// @brief Returns an iterator to the element following the last element of the packet
  /// @note This element acts as a placeholder; attempting to access it results in undefined
  /// behavior
  const_iterator
  cend()
  const noexcept
  {
    return m_buffer.cend();
  }

  /// @brief Checks if the packet has no elements
  /// @note Equivalent to @code begin() == end() @endcode
  bool
  empty()
  const noexcept
  {
    return size() == 0;
  }

  /// @brief Returns the number of bytes in the packet
  /// @note This information is used by the netcode library to know how many bytes are in the
  /// packet. Use resize() if the packet contains less bytes than initially planned.
  /// @note Equivalent to @code std::distance(begin(), end()) @endcode
  size_type
  size()
  const noexcept
  {
    return m_buffer.size() - shift;
  }

  /// @brief Increase the capacity of the packet to a value that's greater or equal to @p new_cap
  /// @note If @p new_cap is greater than the current capacity(), new storage is allocated,
  /// otherwise the method does nothing.
  /// @attention If @p new_cap is greater than capacity(), all iterators and references, including
  /// the end() iterator, are invalidated. Otherwise, no iterators or references are invalidated.
  void
  reserve(size_type new_cap)
  {
    m_buffer.reserve(new_cap + shift);
  }

  /// @brief Returns the number of bytes that the packet has currently allocated space for
  size_type
  capacity()
  const noexcept
  {
    return m_buffer.capacity() - shift;
  }

  /// @brief Requests the removal of unused capacity
  /// @note It is a non-binding request to reduce capacity() to size(). It depends on the
  /// implementation if the request is fulfilled.
  /// @attention All iterators, including the end() iterator, are potentially invalidated
  void
  shrink_to_fit()
  {
    m_buffer.shrink_to_fit();
  }

  /// @brief Removes all bytes from the packet
  /// @note Leaves the capacity() of the packet unchanged
  /// @attention Invalidates any references, pointers, or iterators referring to contained elements.
  /// May invalidate any past-the-end iterators.
  void
  clear()
  {
    m_buffer.resize(shift);
  }

  /// @brief Appends the given byte @p value to the end of the packet
  /// @attention If the new size() is greater than capacity() then all iterators and references,
  /// including the end() iterator are invalidated. Otherwise only the end() iterator is
  /// invalidated.
  void
  push_back(char value)
  {
    m_buffer.push_back(value);
  }

  /// @brief Resizes the packet to contain @p count bytes
  /// @note If size() is greater than @p count, the packet is reduced to its first @p count bytes.
  /// @note If size() is less than @p count, additional bytes won't be initialized
  /// @example_snippet{resize_packet}
  void
  resize(size_type count)
  {
    m_buffer.resize(count + shift);
  }

  /// @brief Resizes the packet to contain @p count bytes with value @p value
  /// @note If size() is greater than @p count, the packet is reduced to its first @p count bytes.
  /// @note If size() is less than @p count, additional bytes will be initialized with @p value
  void
  resize(size_type count, char value)
  {
    m_buffer.resize(count + shift, value);
  }

  /// @internal
  /// @brief Returns a pointer to the position in the packet where the symbol should be when
  /// a source or a repair have been serialized
  const char*
  symbol()
  const noexcept
  {
    return m_buffer.data() + alignment;
  }

  /// @internal
  /// @brief Returns a pointer to the position in the packet where the symbol should be when
  /// a source or a repair have been serialized
  char*
  symbol()
  noexcept
  {
    return m_buffer.data() + alignment;
  }

#ifdef NTC_DUMP_PACKETS
  /// @internal
  void
  dump(std::ostream& os)
  {
    const auto sz = boost::endian::native_to_big(static_cast<std::uint16_t>(m_buffer.size()));
    os.write(reinterpret_cast<const char*>(&sz), sizeof(sz));
    std::copy(std::begin(m_buffer), std::end(m_buffer), std::ostreambuf_iterator<char>(os));
  }

  /// @internal
  void
  load(std::istream& is)
  {
    std::istreambuf_iterator<char> isb(is);
    std::uint16_t sz;
    std::copy_n(isb, 2, reinterpret_cast<char*>(&sz));
    ++isb;
    sz = boost::endian::big_to_native(sz);
    m_buffer.resize(sz);
    std::copy_n(isb, sz, m_buffer.begin());
    ++isb;
  }
#endif

private:

  detail::byte_buffer m_buffer;
};

/*------------------------------------------------------------------------------------------------*/

} // namespace ntc
