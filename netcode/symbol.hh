#pragma once

#include <algorithm>

#include "netcode/source.hh"
#include "netcode/types.hh"

namespace ntc {

/*------------------------------------------------------------------------------------------------*/

class symbol_base
{
public:

  /// @internal
  symbol_buffer_type&
  symbol_buffer()
  noexcept
  {
    return buffer_;
  }

protected:

  /// @brief Tag to discriminate the constructor which reserves a size.
  struct reserve_only{};

  symbol_base(std::size_t buffer_size)
    : buffer_(buffer_size, 0)
  {}

  symbol_base(std::size_t reserve_size, reserve_only)
    : buffer_()
  {
    buffer_.reserve(reserve_size);
  }

  symbol_base(std::size_t len, const char* src)
    : buffer_{}
  {
    buffer_.reserve(len);
    std::copy_n(src, len, buffer_.begin());
  }

  /// @brief Can't delete through this base class.
  ~symbol_base(){}

private:

  symbol_buffer_type buffer_;
};

/*------------------------------------------------------------------------------------------------*/

class symbol
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

  char*
  buffer()
  noexcept
  {
    return symbol_buffer().data();
  }

  void
  resize_buffer(std::size_t new_size)
  {
    symbol_buffer().resize(new_size);
  }
};

/*------------------------------------------------------------------------------------------------*/

class auto_symbol
  : public symbol_base
{
public:

  auto_symbol(std::size_t reserve_size)
    : symbol_base{reserve_size, reserve_only{}}
  {}

  auto_symbol()
    : auto_symbol{256}
  {}

  /// @brief An iterator to write in the symbol buffer.
  using back_insert_iterator = std::back_insert_iterator<symbol_buffer_type>;

  back_insert_iterator
  back_inserter()
  {
    return std::back_inserter(symbol_buffer());
  }
};

/*------------------------------------------------------------------------------------------------*/

} // namespace ntc
