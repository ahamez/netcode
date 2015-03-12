#include <array>
#include <iostream>
#include <iterator>

#include "netcode/galois/field.hh"
#include "netcode/encoder.hh"

struct handler

{
  void
  write(std::size_t len, const char* data)
  {
    std::cout << "write " << len << " bytes\n";
  }
};

int main()
{
  std::array<char[256], 2> src;

  ntc::coding coding{galois::field{8}, [](ntc::id_type x){return x;}};
  ntc::encoder encoder{handler{}, coding, 3};

  {
    auto sym = ntc::symbol{512};
    std::copy(src[0], src[0] + 256, sym.buffer());
    std::copy(src[1], src[1] + 256, sym.buffer() + 256);
    encoder.commit_symbol(std::move(sym));
    std::cout << "---------------\n";
  }
  {
    auto sym = ntc::symbol{512};
    std::copy(src[0], src[0] + 256, sym.buffer());
    std::copy(src[1], src[1] + 256, sym.buffer() + 256);
    encoder.commit_symbol(std::move(sym));
    std::cout << "---------------\n";
  }
  {
    auto sym = ntc::symbol{256};
    std::copy(src[0], src[0] + 256, sym.buffer());
    sym.resize_buffer(512);
    std::copy(src[1], src[1] + 256, sym.buffer() + 256);
    encoder.commit_symbol(std::move(sym));
    std::cout << "---------------\n";
  }
  {
    auto sym = ntc::auto_symbol{512};
    auto inserter = sym.back_inserter();
    std::copy(src[0], src[0] + 256, inserter);
    std::copy(src[1], src[1] + 256, inserter);
    std::copy(src[1], src[1] + 256, inserter);
    encoder.commit_symbol(std::move(sym));
    std::cout << "---------------\n";
  }
  std::cout << encoder.window_size() << '\n';

  return 0;
}
