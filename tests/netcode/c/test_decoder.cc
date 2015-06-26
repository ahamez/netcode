#include <algorithm>

#include "tests/catch.hpp"
#include "tests/netcode/launch.hh"
#include "tests/netcode/c/handlers.h"

#include "netcode/c/decoder.h"

/*------------------------------------------------------------------------------------------------*/

TEST_CASE("create/delete decoder")
{
  launch([](std::uint8_t gf_size)
  {
    context cxt;
    ntc_packet_handler packet_handler = {&cxt, prepare_packet, send_packet};
    ntc_data_handler data_handler = {&cxt, receive_data};

    auto* dec = ntc_new_decoder(gf_size, ntc_in_order, packet_handler, data_handler);
    ntc_delete_decoder(dec);
  });
}

/*------------------------------------------------------------------------------------------------*/
