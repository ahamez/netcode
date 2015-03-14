#include <algorithm> // copy_n

#include "tests/catch.hpp"

#include "netcode/detail/protocol/simple.hh"

/*------------------------------------------------------------------------------------------------*/

using namespace ntc;

/*------------------------------------------------------------------------------------------------*/

namespace {

struct handler
{
  std::size_t bytes_ = 0;
  char data_[2048];

  void
  on_ready_data(std::size_t len, const char* data)
  {
    std::copy_n(data, len, data_ + bytes_);
    bytes_ += len;
  }
};

}

/*------------------------------------------------------------------------------------------------*/

TEST_CASE("An ack is (de)serialized by protocol::simple", "[serialization]" )
{
  detail::handler_derived<handler> h{handler{}};
  detail::protocol::simple serializer{h};

  const detail::ack a_in{{0,1,2,3}};

  serializer.write_ack(a_in);

  REQUIRE(h.handler_.bytes_ == ( sizeof(std::uint8_t)      // id
                               + sizeof(std::uint16_t)     // nb identifiers
                               + 4 * sizeof(std::uint32_t) // identifiers
                               )
         );

  const auto a_out = serializer.read_ack(h.handler_.data_);
  REQUIRE(a_in.source_ids() == a_out.source_ids());
}

/*------------------------------------------------------------------------------------------------*/

TEST_CASE("A repair is (de)serialized by protocol::simple", "[serialization]" )
{
  detail::handler_derived<handler> h{handler{}};
  detail::protocol::simple serializer{h};

//  const detail::ack a_in{{0,1,2,3}};
//
//  serializer.write_ack(a_in);
//
//  REQUIRE(h.handler_.bytes_ == ( sizeof(std::uint8_t)      // id
//                               + sizeof(std::uint16_t)     // nb identifiers
//                               + 4 * sizeof(std::uint32_t) // identifiers
//                               )
//         );
//
//  const auto a_out = serializer.read_ack(h.handler_.data_);
//  REQUIRE(a_in.source_ids() == a_out.source_ids());


}

/*------------------------------------------------------------------------------------------------*/
