#pragma once

#include <iostream>

#include "netcode/coding.hh"
#include "netcode/types.hh"

namespace ntc {

/*------------------------------------------------------------------------------------------------*/

class encoder
{
public:

  /// @brief Constructor
  encoder(coding&& c, unsigned int code_rate, code_type type)
    : coding_{std::move(c)}, rate_{code_rate}, type_{type}
    , current_source_id_{0}, current_repair_id_{0}
    , buffer_size_{1024}
  {}

  /// @brief Constructor
  encoder(coding&& c)
    : encoder{std::move(c), 4, code_type::systematic}
  {}

  void
  start_symbol()
  {
    std::cout << "start_symbol\n";
  }

  void
  end_symbol()
  {
    std::cout << "end_symbol\n";
  }

  char*
  buffer()
  {
    return nullptr;
  }

//  buffer_iterator_type
//  buffer_iterator()
//  {
//    return an iterator to the underlying container for buffer (certainly a std::vector)
//    thus, automatic resizing of buffer will be handler by the std::vector
//  }

  template <typename InputIterator>
  void
  write(InputIterator begin, InputIterator end)
  {

  }

  template <typename InputIterator>
  void
  write_n(InputIterator begin, std::size_t)
  {

  }

  std::size_t& buffer_size()       noexcept {return buffer_size_;}
  std::size_t  buffer_size() const noexcept {return buffer_size_;}

private:

  coding coding_;
  unsigned int rate_; //redundancy
  code_type type_;

  id_type current_source_id_;
  id_type current_repair_id_;

  std::size_t buffer_size_;
};

/*------------------------------------------------------------------------------------------------*/

} // namespace ntc
