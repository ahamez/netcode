#include <algorithm> // copy_n
#include <vector>

#include <catch.hpp>

#include "netcode/detail/packetizer.hh"

/*------------------------------------------------------------------------------------------------*/

using namespace ntc;

/*------------------------------------------------------------------------------------------------*/

namespace /* unnamed */ {

struct handler
{
  packet pkt;

  void
  operator()(const char* data, std::size_t len)
  noexcept
  {
    std::copy_n(data, len, std::back_inserter(pkt));
  }

  void operator()() noexcept {} // end of data
};

} // namespace unnamed

/*------------------------------------------------------------------------------------------------*/

TEST_CASE("An ack is (de)serialized by packetizer")
{
  handler h;
  detail::packetizer<handler> serializer{h};

  const detail::ack a_in{{0,1,2,3}, 33};

  serializer.write_ack(a_in);
  
  const auto a_out = serializer.read_ack(std::move(h.pkt)).first;
  REQUIRE(a_in.source_ids() == a_out.source_ids());
  REQUIRE(a_in.nb_packets() == a_out.nb_packets());
}

/*------------------------------------------------------------------------------------------------*/

TEST_CASE("A repair is (de)serialized by packetizer")
{
  handler h;
  detail::packetizer<handler> serializer{h};

  SECTION("Small number of source ids")
  {
    // The values in the constructor are completely meaningless for this test.
    const detail::encoder_repair r_in{42, 54, {1,2,3,4}, detail::zero_byte_buffer{'a', 'b', 'c'}};
    serializer.write_repair(r_in);

    const auto r_out = serializer.read_repair(std::move(h.pkt)).first;
    REQUIRE(r_in.id() == r_out.id());
    REQUIRE(r_in.source_ids() == r_out.source_ids());
    REQUIRE(r_in.encoded_size() == r_out.encoded_size());
    REQUIRE(std::equal(r_in.symbol().begin(), r_in.symbol().end(), r_out.symbol()));
  }

  SECTION("Large number of source ids")
  {
    auto sl = detail::source_id_list{};
    for (auto i = 0u; i < 1024; ++i)
    {
      sl.insert(i);
    }

    // The values in the constructor are completely meaningless for this test.
    detail::encoder_repair r_in{42, 3, std::move(sl), detail::zero_byte_buffer{'a', 'b', 'c'}};
    serializer.write_repair(r_in);

    const auto r_out = serializer.read_repair(std::move(h.pkt)).first;
    REQUIRE(r_in.id() == r_out.id());
    REQUIRE(r_in.source_ids() == r_out.source_ids());
    REQUIRE(r_in.encoded_size() == r_out.encoded_size());
    REQUIRE(std::equal(r_in.symbol().begin(), r_in.symbol().end(), r_out.symbol()));
  }

  SECTION("Sparse list of source ids")
  {
    // The values in the constructor are completely meaningless for this test.
    const detail::encoder_repair r_in{ 42, 54, {0,1,4,5,6,100,101}
                                     , detail::zero_byte_buffer{'a', 'b', 'c'}};
    serializer.write_repair(r_in);

    const auto r_out = serializer.read_repair(std::move(h.pkt)).first;
    REQUIRE(r_in.id() == r_out.id());
    REQUIRE(r_in.source_ids() == r_out.source_ids());
    REQUIRE(r_in.encoded_size() == r_out.encoded_size());
    REQUIRE(std::equal(r_in.symbol().begin(), r_in.symbol().end(), r_out.symbol()));
  }

  SECTION("Big values")
  {
    const auto base = 1 << 21;
    const detail::encoder_repair r_in{ 1 << 20, 54
                                     , {base, base + 1, base + 2, base + 100, base + 101}
                                     , detail::zero_byte_buffer{'a', 'b', 'c'}};
    serializer.write_repair(r_in);

    const auto r_out = serializer.read_repair(std::move(h.pkt)).first;
    REQUIRE(r_in.id() == r_out.id());
    REQUIRE(r_in.source_ids() == r_out.source_ids());
    REQUIRE(r_in.encoded_size() == r_out.encoded_size());
    REQUIRE(std::equal(r_in.symbol().begin(), r_in.symbol().end(), r_out.symbol()));
  }

  SECTION("Repair with only one source")
  {
    const detail::encoder_repair r_in{ 0, 33, {4242}, detail::zero_byte_buffer{'x'}};
    serializer.write_repair(r_in);

    const auto r_out = serializer.read_repair(std::move(h.pkt)).first;
    REQUIRE(r_in.id() == r_out.id());
    REQUIRE(r_in.source_ids() == r_out.source_ids());
    REQUIRE(r_in.encoded_size() == r_out.encoded_size());
    REQUIRE(std::equal(r_in.symbol().begin(), r_in.symbol().end(), r_out.symbol()));
  }
}

/*------------------------------------------------------------------------------------------------*/

TEST_CASE("A source is (de)serialized by packetizer")
{
  handler h;
  detail::packetizer<handler> serializer{h};

  const detail::encoder_source s_in{394839, {'a', 'b', 'c', 'd'}};

  serializer.write_source(s_in);

  REQUIRE(h.pkt.size() == ( sizeof(std::uint8_t)      // type
                          + sizeof(std::uint32_t)     // id
                          + sizeof(std::uint16_t)     // user symbol size
                          + 4                         // symbol
                          ));

  const auto s_out = serializer.read_source(std::move(h.pkt)).first;
  REQUIRE(s_in.id() == s_out.id());
  REQUIRE(s_in.size() == s_out.symbol_size());
  REQUIRE(std::equal(s_in.symbol().begin(), s_in.symbol().end(), s_out.symbol()));
}

/*------------------------------------------------------------------------------------------------*/

TEST_CASE("Prevent buffer overflow")
{
  handler h;
  detail::packetizer<handler> serializer{h};

  SECTION("repair")
  {
    std::vector<char> crafted(512, 0);

    // We need a valid packet type.
    crafted[0] = static_cast<std::uint8_t>(detail::packet_type::repair);

    // Write symbol size > packet size
    const auto symbol_size = boost::endian::native_to_big(static_cast<std::uint16_t>(1024));

    const auto symbol_size_addr = reinterpret_cast<const char*>(&symbol_size);
    crafted[5] = symbol_size_addr[0];
    crafted[6] = symbol_size_addr[1];

    REQUIRE_THROWS_AS(serializer.read_repair(packet{begin(crafted), end(crafted)}), overflow_error);
  }

  SECTION("source")
  {
    SECTION("Much larger")
    {
      std::vector<char> crafted(512, 0);

      // We need a valid packet type.
      crafted[0] = static_cast<std::uint8_t>(detail::packet_type::source);

      // Write symbol size > packet size
      const auto symbol_size = boost::endian::native_to_big(static_cast<std::uint16_t>(1024));

      const auto symbol_size_addr = reinterpret_cast<const char*>(&symbol_size);
      crafted[5] = symbol_size_addr[0];
      crafted[6] = symbol_size_addr[1];

      REQUIRE_THROWS_AS( serializer.read_source(packet{begin(crafted), end(crafted)})
                       , overflow_error);
    }

    SECTION("Exact size")
    {
      std::vector<char> crafted(512, 0);

      // We need a valid packet type.
      crafted[0] = static_cast<std::uint8_t>(detail::packet_type::source);

      // Maximal symbol size (7 == source's headers size)
      const auto symbol_size = boost::endian::native_to_big(static_cast<std::uint16_t>(512-7));

      const auto symbol_size_addr = reinterpret_cast<const char*>(&symbol_size);
      crafted[5] = symbol_size_addr[0];
      crafted[6] = symbol_size_addr[1];
      REQUIRE_NOTHROW(serializer.read_source(packet{begin(crafted), end(crafted)}));
    }

    SECTION("Exact size + 1")
    {
      std::vector<char> crafted(512, 0);

      // We need a valid packet type.
      crafted[0] = static_cast<std::uint8_t>(detail::packet_type::source);

      // Maximal symbol size (7 == source's headers size)
      const auto symbol_size = boost::endian::native_to_big(static_cast<std::uint16_t>(512-7 + 1));

      const auto symbol_size_addr = reinterpret_cast<const char*>(&symbol_size);
      crafted[5] = symbol_size_addr[0];
      crafted[6] = symbol_size_addr[1];
      REQUIRE_THROWS_AS( serializer.read_source(packet{begin(crafted), end(crafted)})
                       , overflow_error);
    }
  }
}

/*------------------------------------------------------------------------------------------------*/
