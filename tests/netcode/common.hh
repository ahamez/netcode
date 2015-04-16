#pragma once

#include "netcode/detail/source_list.hh"

namespace /* unnamed */ {

/*------------------------------------------------------------------------------------------------*/

using namespace ntc;

/*------------------------------------------------------------------------------------------------*/

inline
const detail::source&
add_source(detail::source_list& sl,std::uint32_t id, detail::byte_buffer&& buf, std::size_t sz)
{
  return sl.emplace(id, std::move(buf), sz);
}

/*------------------------------------------------------------------------------------------------*/

struct packet_handler
{
  // Stores all packets.
  std::vector<std::vector<char>> vec;

  packet_handler()
    : vec(1)
  {}

  void
  operator()(const char* src, std::size_t len)
  {
    std::copy_n(src, len, std::back_inserter(vec.back()));
  }

  void
  operator()()
  {
    vec.emplace_back();
  }

  std::size_t
  nb_packets()
  const noexcept
  {
    return vec.size() - 1;
  }
};

/*------------------------------------------------------------------------------------------------*/

struct data_handler
{
  // Stores all symbols.
  std::vector<std::vector<char>> vec;

  data_handler()
    : vec()
  {}

  void
  operator()(const char* src, std::size_t len)
  {
    vec.emplace_back(src, src + len);
  }
};

/*------------------------------------------------------------------------------------------------*/

} // namespace unnamed
