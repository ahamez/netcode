#include <algorithm>

#include "tests/catch.hpp"

#include "netcode/decoder.hh"
#include "netcode/encoder.hh"

/*------------------------------------------------------------------------------------------------*/

using namespace ntc;

/*------------------------------------------------------------------------------------------------*/

namespace /* unnamed */ {

struct my_handler
{
  char* data;
  std::size_t written;

  void
  operator()(const char* src, std::size_t len)
  {
    std::copy_n(src + written, len, data);
    written += len;
  }
};


} // namespace unnamed

/*------------------------------------------------------------------------------------------------*/

TEST_CASE("")
{
  char network_to_decoder[2048];
  char network_to_encoder[2048];
  char decoded_symbol[2048];

  ntc::encoder encoder{my_handler{network_to_decoder, 0ul}};
  ntc::decoder decoder{my_handler{network_to_encoder, 0ul}, my_handler{decoded_symbol, 0ul}};

  const auto s0 = {'a', 'b', 'c'};

  encoder.commit(copy_symbol{begin(s0), end(s0)});

//  decoder.notify(copy_packet(<#ntc::copy_packet &&#>))
}

/*------------------------------------------------------------------------------------------------*/
