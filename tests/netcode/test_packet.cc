#include <algorithm>

#include "tests/catch.hpp"

#include "netcode/packet.hh"

/*------------------------------------------------------------------------------------------------*/

using namespace ntc;

/*------------------------------------------------------------------------------------------------*/

TEST_CASE("Empty packet")
{
  packet p;
  REQUIRE(p.empty());
  REQUIRE(p.size() == 0);
  REQUIRE(p.begin() == p.end());

  SECTION("push back")
  {
    p.push_back('x');
    REQUIRE(p.size() == 1);
    REQUIRE(not p.empty());
    REQUIRE(p[0] == 'x');
    REQUIRE(p.data()[0] == 'x');
    REQUIRE((p.begin() + 1) == p.end());
  }
}

/*------------------------------------------------------------------------------------------------*/

TEST_CASE("Packet from other source")
{
  auto init = {'a','b','c','d','e','f','g','h','i','j','k','l','m','n','o','p','q','r','s','t','u'};
  SECTION("initializer list")
  {
    packet p{init};
    REQUIRE(p.size() == 21);
    auto cpt = 0ul;
    for (auto cit = p.begin(); cit != p.end(); ++cit)
    {
      REQUIRE(p[cpt] == *cit);
      REQUIRE(p[cpt] == 'a' + cpt);
      REQUIRE(p.data()[cpt] == 'a' + cpt);
      cpt += 1;
    }
  }

  SECTION("iterable")
  {
    packet p{std::begin(init), std::end(init)};
    REQUIRE(p.size() == 21);
    auto cpt = 0ul;
    for (auto cit = p.begin(); cit != p.end(); ++cit)
    {
      REQUIRE(p[cpt] == *cit);
      REQUIRE(p[cpt] == 'a' + cpt);
      REQUIRE(p.data()[cpt] == 'a' + cpt);
      cpt += 1;
    }
  }
}

/*------------------------------------------------------------------------------------------------*/

TEST_CASE("Clearing packet")
{
  packet p{0,1,2,3};
  REQUIRE(p.size() == 4);
  p.clear();
  REQUIRE(p.size() == 0);
}

/*------------------------------------------------------------------------------------------------*/

TEST_CASE("Construction with size")
{
  SECTION("no value")
  {
    const packet p(10);
    REQUIRE(p.size() == 10);
  }

  SECTION("value")
  {
    const packet p(10, 'x');
    REQUIRE(p.size() == 10);
    REQUIRE(std::all_of(p.begin(), p.end(), [](char x){return x == 'x';}));
    REQUIRE(std::all_of(p.data(), p.data() + p.size(), [](char x){return x == 'x';}));
  }
}

/*------------------------------------------------------------------------------------------------*/

TEST_CASE("Packet resizing")
{
  packet p{0,1,2,3};
  REQUIRE(p.size() == 4);

  SECTION("no value")
  {
    SECTION("smaller new size")
    {
      p.resize(2);
      REQUIRE(p.size() == 2);
      REQUIRE(p[0] == 0);
      REQUIRE(p[1] == 1);
    }

    SECTION("bigger new size")
    {
      p.resize(10);
      REQUIRE(p.size() == 10);
      REQUIRE(p[0] == 0);
      REQUIRE(p[1] == 1);
      REQUIRE(p[2] == 2);
      REQUIRE(p[3] == 3);
    }
  }

  SECTION("value")
  {
    SECTION("smaller new size")
    {
      p.resize(2, 'x');
      REQUIRE(p.size() == 2);
      REQUIRE(p[0] == 0);
      REQUIRE(p[1] == 1);
    }

    SECTION("bigger new size")
    {
      p.resize(10, 'x');
      REQUIRE(p.size() == 10);
      REQUIRE(p[0] == 0);
      REQUIRE(p[1] == 1);
      REQUIRE(p[2] == 2);
      REQUIRE(p[3] == 3);
      REQUIRE(std::all_of(p.begin() + 4, p.end(), [](char x){return x == 'x';}));
    }
  }
}

/*------------------------------------------------------------------------------------------------*/
