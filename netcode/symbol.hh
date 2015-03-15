#pragma once

#include <algorithm> // copy_n
#include <iterator>  // back_inserter

#include "netcode/detail/multiple.hh"
#include "netcode/detail/symbol_base.hh"
#include "netcode/detail/symbol_buffer.hh"

namespace ntc {

/*------------------------------------------------------------------------------------------------*/

/// @brief A symbol with an allocated buffer.
///
/// @attention Because it's not possible for the library to keep track of the number of written
/// bytes, when the symbol is completely written in the buffer, set_user_size() must be called.
/// This method indicates how many bytes of the allocated buffer are really used. In debug mode, an
/// assertion will be raised if it's not the case.
/// @note It's possible to resize the buffer using resize_buffer().
class symbol final
  : public detail::symbol_base
{
public:

  /// @brief Constructor.
  /// @param size The size of the buffer to allocate.
  symbol(std::size_t size)
    : symbol_base{size}
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

  /// @brief Tell the library how many bytes were written in the buffer.
  void
  set_user_size(std::size_t sz)
  noexcept
  {
    symbol_base::user_size_ = sz;
  }
};

/*------------------------------------------------------------------------------------------------*/

/// @brief A symbol which automatically grows as needed.
///
/// It provides the easiest way to avoid copying, and can be used with STL algorithms.
class auto_symbol final
  : public detail::symbol_base
{
public:

  /// @brief Construct with a reserved buffer to avoid memory re-allocations when growing.
  /// @param reserve_size The size to reserve.
  auto_symbol(std::size_t reserve_size)
    : symbol_base{reserve_size, reserve_only{}}
  {}

  /// @brief Default constructor with a default size (512 bytes) for the reserved buffer.
  auto_symbol()
    : auto_symbol{512}
  {}

  /// @brief An iterator to write in the symbol buffer.
  using back_insert_iterator = std::back_insert_iterator<detail::symbol_buffer>;

  /// @brief Get a back inserter iterator to the symbol buffer.
  ///
  /// The returned iterator is usable as an output iterator with any STL algorithm.
  /// @note Inserting a number of times greater than the reserved size will result in a new
  /// allocation, and possibly a copy. Thus, the reserved size given at construction should be
  /// large enough to avoid this situation.
  back_insert_iterator
  back_inserter()
  {
    return std::back_inserter(symbol_base::buffer());
  }

  /// @brief The encoder needs to set the user size.
  friend class encoder;
};

/*------------------------------------------------------------------------------------------------*/

/// @brief A symbol that copies the input data.
///
/// Use this symbol when the data already exists and must be copied, otherwise auto_symbol and
/// @ref symbol sould be prefered.
class copy_symbol final
  : public detail::symbol_base
{
public:

  /// @brief Constructor.
  /// @param len The size of the data to copy.
  /// @param src The address of the data to copy.
  copy_symbol(std::size_t len, const char* src)
    : symbol_base{len, src}
  {}
};

/*------------------------------------------------------------------------------------------------*/

} // namespace ntc
