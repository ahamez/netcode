#include <algorithm>

#include "tests/catch.hpp"

#include "netcode/decoder.hh"

/*------------------------------------------------------------------------------------------------*/

using namespace ntc;

/*------------------------------------------------------------------------------------------------*/

namespace /* unnamed */ {

struct dummy_handler
{
  void
  on_ready_data(std::size_t, const char*)
  {}
};

} // namespace unnamed

/*------------------------------------------------------------------------------------------------*/

TEST_CASE("Decoder construction", "[decoder]")
{
  ntc::decoder decoder{dummy_handler{}};
}

/*------------------------------------------------------------------------------------------------*/
