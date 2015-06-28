#include <new> // nothrow

#include "netcode/c/detail/check_error.hh"
#include "netcode/c/packet.h"

/*------------------------------------------------------------------------------------------------*/

ntc_packet_t*
ntc_new_packet(uint16_t size)
noexcept
{
  return new (std::nothrow) ntc_packet_t(size);
}

/*------------------------------------------------------------------------------------------------*/

ntc_packet_t*
ntc_new_packet_from(const char* src, uint16_t size)
noexcept
{
  return new (std::nothrow) ntc_packet_t(src, src + size);
}

/*------------------------------------------------------------------------------------------------*/

void
ntc_delete_packet(ntc_packet_t* packet)
noexcept
{
  delete packet;
}

/*------------------------------------------------------------------------------------------------*/

char*
ntc_packet_buffer(ntc_packet_t* packet)
noexcept
{
  return packet->data();
}

/*------------------------------------------------------------------------------------------------*/

uint16_t
ntc_packet_get_size(const ntc_packet_t* packet)
noexcept
{
  return static_cast<uint16_t>(packet->size());
}

/*------------------------------------------------------------------------------------------------*/

void
ntc_packet_resize(ntc_packet_t* packet, uint16_t new_size, ntc_error* error)
noexcept
{
  ntc::detail::check_error([&]{packet->resize(new_size);}, error);
}

/*------------------------------------------------------------------------------------------------*/
