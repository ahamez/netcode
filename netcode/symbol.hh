#pragma once

#include <algorithm> // copy_n
#include <iterator>  // back_inserter

#include "netcode/detail/multiple.hh"
#include "netcode/detail/symbol_base.hh"
#include "netcode/detail/symbol_buffer.hh"

namespace ntc {

/*------------------------------------------------------------------------------------------------*/

/// @brief A symbol to be encoded.
class symbol final
  : public detail::symbol_base
{
public:

  /// @brief Allocate a buffer.
  /// @param size The size of the buffer.
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
    return symbol_base::buffer().data();
  }

  /// @brief Resize the buffer.
  /// @param size The new buffer size.
  ///
  /// May cause a copy.
  void
  resize_buffer(std::size_t size)
  {
    symbol_base::buffer().resize(size);
  }
};

/*------------------------------------------------------------------------------------------------*/

/// @brief A symbol to be encoded which automatically grows as needed.
class auto_symbol final
  : public detail::symbol_base
{
public:

  /// @brief Construct with a reserved buffer to avoid memory re-allocations when growing.
  /// @param reserve_size The size to reserve.
  auto_symbol(std::size_t reserve_size)
    : symbol_base{reserve_size < 256 ? 256 : reserve_size, reserve_only{}}
  {}

  /// @brief Defaut constructor with a default size for the reserved buffer.
  auto_symbol()
    : auto_symbol{256}
  {}

  /// @brief An iterator to write in the symbol buffer.
  using back_insert_iterator = std::back_insert_iterator<detail::symbol_buffer>;

  /// @brief Get a back inserter iterator to the symbol buffer.
  back_insert_iterator
  back_inserter()
  {
    return std::back_inserter(symbol_base::buffer());
  }
};

/*------------------------------------------------------------------------------------------------*/

} // namespace ntc
