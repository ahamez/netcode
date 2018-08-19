#pragma once

#include <algorithm> // copy
#include <iostream>

#include <boost/endian/conversion.hpp>

#include "netcode/packet.hh"

namespace ntc { namespace detail {

/*------------------------------------------------------------------------------------------------*/

/// @internal
struct serialize_packet
{
  static
  void
  write(std::ostream& os, const packet& p)
  {
    const auto sz = boost::endian::native_to_big(static_cast<std::uint16_t>(p.m_buffer.size()));
    os.write(reinterpret_cast<const char*>(&sz), sizeof(sz));
    std::copy(p.m_buffer.begin(), p.m_buffer.end(), std::ostreambuf_iterator<char>(os));
  }

  static
  packet
  read(std::istream& is)
  {
    auto p = packet{};

    std::istreambuf_iterator<char> isb(is);
    auto sz = std::uint16_t{};
    std::copy_n(isb, 2, reinterpret_cast<char*>(&sz));
    ++isb;

    const auto sz_native = boost::endian::big_to_native(sz);

    p.m_buffer.resize(sz_native);
    std::copy_n(isb, sz_native, p.m_buffer.begin());

    ++isb;
    return p;
  }
};

/*------------------------------------------------------------------------------------------------*/

}} // namespace ntc::detail
