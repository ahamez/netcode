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
  char data[2048];
  std::size_t written = 0;

  void
  operator()(const char* src, std::size_t len)
  {
    if (src)
    {
      std::copy_n(src, len, data + written);
      written += len;
    }
  }
};


} // namespace unnamed

/*------------------------------------------------------------------------------------------------*/

TEST_CASE("Decoder gives a correct source to user")
{
  ntc::encoder enc{my_handler{}};
  ntc::decoder dec{my_handler{}, my_handler{}};

  auto& enc_handler = *enc.data_handler().target<my_handler>();
  auto& dec_handler = *dec.symbol_handler().target<my_handler>();

  const auto s0 = {'a', 'b', 'c'};

  enc.commit(copy_symbol{begin(s0), end(s0)});

  // Send serialized data to decoder.
  dec.notify(copy_packet{enc_handler.data, enc_handler.written});

  REQUIRE(dec_handler.written == s0.size());
  REQUIRE(std::equal(begin(s0), end(s0), dec_handler.data));
}

/*------------------------------------------------------------------------------------------------*/
