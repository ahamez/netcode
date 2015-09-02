#include <algorithm>

#include <catch.hpp>

#include "netcode/c/data.h"

/*------------------------------------------------------------------------------------------------*/

TEST_CASE("create/delete data")
{
  char src[4] = {'a', 'b', 'c', 'u'};
  ntc_data_t* data = ntc_new_data(1024);
  std::copy_n(src, 4, ntc_data_buffer(data));
  REQUIRE(ntc_data_get_size(data) >= 4);
  ntc_error error;
  ntc_data_resize(data, 4, &error);
  REQUIRE(ntc_data_get_size(data) == 4);
  REQUIRE(std::equal(src, src + 4, ntc_data_buffer(data)));
  ntc_delete_data(data);
}

/*------------------------------------------------------------------------------------------------*/

TEST_CASE("create data from copy")
{
  char src[4] = {'a', 'b', 'c', 'u'};
  ntc_data_t* data = ntc_new_data_from(src, 4);
  REQUIRE(std::equal(src, src + 4, ntc_data_buffer(data)));
  REQUIRE(ntc_data_get_size(data) == 4);
  ntc_delete_data(data);
}

/*------------------------------------------------------------------------------------------------*/

TEST_CASE("resize data")
{
  char src[4] = {'a', 'b', 'c', 'u'};
  ntc_data_t* data = ntc_new_data_from(src, 4);
  ntc_error error;
  ntc_data_resize(data, 10, &error);
  REQUIRE(ntc_data_get_size(data) == 10);
  ntc_delete_data(data);
}

/*------------------------------------------------------------------------------------------------*/
