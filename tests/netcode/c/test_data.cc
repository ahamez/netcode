#include <algorithm>

#include "tests/catch.hpp"

#include "netcode/c/data.h"

/*------------------------------------------------------------------------------------------------*/

TEST_CASE("create/delete data")
{
  char src[4] = {'a', 'b', 'c', 'u'};
  ntc_data_t* data = ntc_new_data(1024);
  std::copy_n(src, 4, ntc_data_buffer(data));
  REQUIRE(ntc_data_get_reserved_size(data) >= 4);
  ntc_data_set_used_bytes(data, 4);
  REQUIRE(ntc_data_get_used_bytes(data) == 4);
  REQUIRE(std::equal(src, src + 4, ntc_data_buffer(data)));
  ntc_delete_data(data);
}

/*------------------------------------------------------------------------------------------------*/

TEST_CASE("create data from copy")
{
  char src[4] = {'a', 'b', 'c', 'u'};
  ntc_data_t* data = ntc_new_data_copy(src, 4);
  REQUIRE(std::equal(src, src + 4, ntc_data_buffer(data)));
  REQUIRE(ntc_data_get_used_bytes(data) == 4);
  ntc_delete_data(data);
}

/*------------------------------------------------------------------------------------------------*/

TEST_CASE("reset data")
{
  char src[4] = {'a', 'b', 'c', 'u'};
  ntc_data_t* data = ntc_new_data_copy(src, 4);
  ntc_error error;
  ntc_data_reset(data, 10, &error);
  REQUIRE(ntc_data_get_used_bytes(data) == 0);
  ntc_delete_data(data);
}

/*------------------------------------------------------------------------------------------------*/
