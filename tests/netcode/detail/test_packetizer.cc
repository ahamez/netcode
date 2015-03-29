#include <algorithm> // copy_n

#include "tests/catch.hpp"

#include "netcode/detail/packetizer.hh"

/*------------------------------------------------------------------------------------------------*/

using namespace ntc;

/*------------------------------------------------------------------------------------------------*/

namespace /* unnamed */ {

struct my_handler
{
  std::size_t bytes_ = 0;
  char data_[2048];

  void
  operator()(const char* data, std::size_t len)
  noexcept
  {
    if (data)
    {
      std::copy_n(data, len, data_ + bytes_);
      bytes_ += len;
    }
  }
};

// Ease the access to the callable stored in a std::function.
const my_handler&
target(const handler& h)
{
  return *h.target<my_handler>();
}

} // namespace unnamed

/*------------------------------------------------------------------------------------------------*/

TEST_CASE("An ack is (de)serialized by packetizer")
{
  handler h = my_handler{};
  detail::packetizer serializer{h};

  const detail::ack a_in{{0,1,2,3}};

  serializer.write_ack(a_in);

  REQUIRE(target(h).bytes_ == ( sizeof(std::uint8_t)      // type
                              + sizeof(std::uint16_t)     // nb nb source ids
                              + 4 * sizeof(std::uint32_t) // identifiers
                              ));

  const auto a_out = serializer.read_ack(target(h).data_);
  REQUIRE(a_in.source_ids() == a_out.source_ids());
}

/*------------------------------------------------------------------------------------------------*/

TEST_CASE("A repair is (de)serialized by packetizer")
{
  handler h = my_handler{};
  detail::packetizer serializer{h};

  // The values in the constructor are completely meaningless for this test.
  const detail::repair r_in{42, 3, {0,1,2,3}, detail::zero_byte_buffer{'a', 'b', 'c'}};
  serializer.write_repair(r_in);

  REQUIRE(target(h).bytes_ == ( sizeof(std::uint8_t)      // type
                              + sizeof(std::uint32_t)     // id
                              + sizeof(std::uint16_t)     // nb source ids
                              + 4 * sizeof(std::uint32_t) // identifiers
                              + sizeof(std::uint16_t)     // encoded size
                              + sizeof(std::uint16_t)     // symbol length
                              + 3                         // symbol
                              ));

  const auto r_out = serializer.read_repair(target(h).data_);
  REQUIRE(r_in.id() == r_out.id());
  REQUIRE(r_in.source_ids() == r_out.source_ids());
  REQUIRE(r_in.size() == r_out.size());
  REQUIRE(r_in.buffer() == r_out.buffer());
}

/*------------------------------------------------------------------------------------------------*/

TEST_CASE("A source is (de)serialized by packetizer")
{
  handler h = my_handler{};
  detail::packetizer serializer{h};

  const detail::source s_in{394839, detail::byte_buffer{'a', 'b', 'c', 'd'}, 4};

  serializer.write_source(s_in);

  REQUIRE(target(h).bytes_ == ( sizeof(std::uint8_t)      // type
                              + sizeof(std::uint32_t)     // id
                              + sizeof(std::uint16_t)     // user symbol size
                              + 4                         // symbol
                              ));

  const auto s_out = serializer.read_source(target(h).data_);
  REQUIRE(s_in.id() == s_out.id());
  REQUIRE(s_in.user_size() == s_out.user_size());
  REQUIRE(s_in.buffer() == s_out.buffer());
}

/*------------------------------------------------------------------------------------------------*/
