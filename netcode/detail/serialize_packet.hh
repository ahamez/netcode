#pragma once

#include <algorithm> // copy

#include <iostream>
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wconversion"
#include <boost/endian/conversion.hpp>
#pragma GCC diagnostic pop

#include "netcode/packet.hh"

namespace ntc { namespace detail {

/*------------------------------------------------------------------------------------------------*/

/// @internal
struct serialize_packet
{
  static
  void
  write(std::ostream& os, const packet& p, bool compact = true)
  {
    if (compact)
    {
      const auto sz = boost::endian::native_to_big(static_cast<std::uint16_t>(p.size()));
      os.write(reinterpret_cast<const char*>(&sz), sizeof(sz));
      std::copy(p.begin(), p.end(), std::ostreambuf_iterator<char>(os));
    }
    else
    {
      const auto sz = boost::endian::native_to_big(static_cast<std::uint16_t>(p.m_buffer.size()));
      os.write(reinterpret_cast<const char*>(&sz), sizeof(sz));
      std::copy(p.m_buffer.begin(), p.m_buffer.end(), std::ostreambuf_iterator<char>(os));
    }
  }

  static
  packet
  read(std::istream& is, bool compact = true)
  {
    packet p;

    std::istreambuf_iterator<char> isb(is);
    std::uint16_t sz;
    std::copy_n(isb, 2, reinterpret_cast<char*>(&sz));
    ++isb;
    sz = boost::endian::big_to_native(sz);

    if (compact)
    {
      p.resize(sz);
      std::copy_n(isb, sz, p.begin());
    }
    else
    {
      p.m_buffer.resize(sz);
      std::copy_n(isb, sz, p.m_buffer.begin());
    }

    ++isb;
    return p;
  }
};

/*------------------------------------------------------------------------------------------------*/

}} // namespace ntc::detail
