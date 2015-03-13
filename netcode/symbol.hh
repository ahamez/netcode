#pragma once

#include <algorithm> // copy_n
#include <iterator>  // back_inserter

#include "netcode/detail/types.hh"

namespace ntc {

/*------------------------------------------------------------------------------------------------*/

/// @brief The base class for symbol, not intended for direct usage.
class symbol_base
{
public:

  /// @internal
  detail::symbol_buffer_type&
  symbol_buffer()
  noexcept
  {
    return buffer_;
  }

protected:

  /// @brief Tag to discriminate the constructor which reserves a size.
  struct reserve_only{};

  /// @brief Zero-out the buffer.
  /// @param buffer_size The buffer size.
  symbol_base(std::size_t buffer_size)
    : buffer_(buffer_size, 0)
  {}

  /// @brief Reserve memory for the buffer.
  /// @param reserve_size The size to reserve.
  symbol_base(std::size_t reserve_size, reserve_only)
    : buffer_()
  {
    buffer_.reserve(reserve_size);
  }

  /// @brief Initialize the buffer with a copy of a memory location.
  /// @param len The size of the data.
  /// @param src The data to copy.
  symbol_base(std::size_t len, const char* src)
    : buffer_(src, src + len)
  {}

  /// @brief Can't delete through this base class.
  ~symbol_base(){}

private:

  /// @brief The buffer storage.
  detail::symbol_buffer_type buffer_;
};

/*------------------------------------------------------------------------------------------------*/

/// @brief A symbol to be encoded.
class symbol final
  : public symbol_base
{
public:

  /// @brief Construct with a 0-initialized buffer.
  /// @param size The size of the initialized buffer.
  symbol(std::size_t size)
    : symbol_base{size}
  {}

  /// @brief Construct with a copy of some data.
  /// @param len The size of the data to copy.
  /// @param src The address of the data to copy.
  symbol(std::size_t len, const char* src)
    : symbol_base{len, src}
  {}

  /// @brief Get the buffer where to write the symbol.
  char*
  buffer()
  noexcept
  {
    return symbol_buffer().data();
  }

  /// @brief Resize the buffer.
  /// @param size The new buffer size.
  ///
  /// May cause a copy.
  void
  resize_buffer(std::size_t size)
  {
    symbol_buffer().resize(size);
  }
};

/*------------------------------------------------------------------------------------------------*/

/// @brief A symbol to be encoded which automatically grows as needed.
class auto_symbol final
  : public symbol_base
{
public:

  /// @brief Construct with a pre-allocated buffer to avoid memory re-allocations.
  auto_symbol(std::size_t reserve_size)
    : symbol_base{reserve_size, reserve_only{}}
  {}

  /// @brief Defaut constructor.
  auto_symbol()
    : auto_symbol{256}
  {}

  /// @brief An iterator to write in the symbol buffer.
  using back_insert_iterator = std::back_insert_iterator<detail::symbol_buffer_type>;

  /// @brief Get a back inserter iterator to the symbol buffer.
  back_insert_iterator
  back_inserter()
  {
    return std::back_inserter(symbol_buffer());
  }
};

/*------------------------------------------------------------------------------------------------*/

} // namespace ntc
