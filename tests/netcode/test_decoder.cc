#include <algorithm>

#include "tests/catch.hpp"

#include "netcode/decoder.hh"
#include "netcode/encoder.hh"

/*------------------------------------------------------------------------------------------------*/

using namespace ntc;

/*------------------------------------------------------------------------------------------------*/

namespace /* unnamed */ {

struct handler
{
  char* data;
  std::size_t nb_written;

  void
  on_data(const char*, std::size_t)
  {}

  void
  on_symbol(const char*, std::size_t)
  {}
};

} // namespace unnamed

/*------------------------------------------------------------------------------------------------*/

TEST_CASE("")
{
  ntc::encoder encoder{handler{}};
  ntc::decoder decoder{handler{}};

  const auto s0 = {'a', 'b', 'c'};

  encoder.commit(copy_symbol{begin(s0), end(s0)});
}

/*------------------------------------------------------------------------------------------------*/
