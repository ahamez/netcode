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
//  const auto* dec = ntc_new_decoder(gf_size, <#ntc_ordering_type order#>, <#ntc_packet_handler packet_handler#>, <#ntc_data_handler data_handler#>)

    context cxt;
    ntc_packet_handler packet_handler = {&cxt, prepare_packet, send_packet};
  });
}

/*------------------------------------------------------------------------------------------------*/
