#pragma once

#include "netcode/detail/repair.hh"
#include "netcode/detail/source_list.hh"
#include "netcode/packet.hh"

namespace /* unnamed */ {

/*------------------------------------------------------------------------------------------------*/

using namespace ntc;

/*------------------------------------------------------------------------------------------------*/

// Convert an encoder repair to a decoder repair
inline
detail::decoder_repair
mk_decoder_repair(const detail::encoder_repair& r)
{
  return { r.id(), r.encoded_size(), detail::source_id_list{r.source_ids()}
         , r.symbol(), r.symbol().size()};
}

/*------------------------------------------------------------------------------------------------*/

inline
const detail::encoder_source&
add_source(detail::source_list& sl,std::uint32_t id, detail::byte_buffer&& buf)
{
  return sl.emplace(id, std::move(buf));
}

/*------------------------------------------------------------------------------------------------*/

class packet_handler
{
public:

  packet_handler()
    : m_vec(1)
  {}

  void
  operator()(const char* src, std::size_t len)
  {
    std::copy_n(src, len, std::back_inserter(m_vec.back()));
  }

  void
  operator()()
  {
    m_vec.emplace_back();
  }

  const packet&
  operator[](std::size_t pos)
  const noexcept
  {
    return m_vec[pos];
  }

  std::size_t
  nb_packets()
  const noexcept
  {
    return m_vec.size() - 1;
  }

private:

  // Stores all packets.
  std::vector<packet> m_vec;
};

/*------------------------------------------------------------------------------------------------*/

class data_handler
{
public:

  data_handler()
    : m_vec()
  {}

  void
  operator()(const char* src, std::size_t len)
  {
    m_vec.emplace_back(src, src + len);
  }

  std::vector<char>
  operator[](std::size_t pos)
  const noexcept
  {
    return m_vec[pos];
  }

  std::size_t
  nb_data()
  const noexcept
  {
    return m_vec.size();
  }

private:

  // Stores all symbols.
  std::vector<std::vector<char>> m_vec;
};

/*------------------------------------------------------------------------------------------------*/

} // namespace unnamed
