#pragma once

#include <iostream>

#include "netcode/coding.hh"
#include "netcode/symbol.hh"
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
    , buffer_size_(1024)
  {}

  /// @brief Constructor
  encoder(coding&& c)
    : encoder{std::move(c), 4, code_type::systematic}
  {}

  encoder&
  operator<<(symbol_start s)
  {
    start_symbol();
    return *this;
  }

  encoder&
  operator<<(symbol_end)
  {
    end_symbol();
    return *this;
  }

  encoder&
  operator<<(symbol_part part)
  {
    std::cout << "will read " << (part.buffer_end - part.buffer_start) << " bytes\n";
    return *this;
  }

  encoder&
  operator<<(char byte)
  {
    return *this;
  }

  void
  start_symbol()
  {
    std::cout << "start_symbol\n";
  }

  void
  end_symbol()
  {
    std::cout << "end_symbol " << "\n";
  }

  char*
  buffer()
  {
    return nullptr;
  }

  template <typename InputIterator>
  void
  write(InputIterator begin, InputIterator end)
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
