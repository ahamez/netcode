#include <algorithm>
#include <array>
#include <iostream>

#include "netcode/galois/field.hh"
#include "netcode/encoder.hh"

struct handler
{
  void
  on_ready_data(std::size_t len, const char* data)
  {
    std::cout << "write " << len << " bytes\n";
  }
};

int main()
{
  char src[1024];

  ntc::coding coding{galois::field{8}, [](ntc::id_type x){return x;}};
  ntc::encoder encoder{handler{}, coding, 3};

  std::cout << "---------------\n";
  {
    auto sym = ntc::symbol{512};
    std::copy(src      , src + 256, sym.buffer());
    std::copy(src + 256, src + 512, sym.buffer() + 256);
    encoder.commit(std::move(sym));
  }
  std::cout << "---------------\n";
  {
    auto sym = ntc::symbol{512};
    std::copy(src, src + 512, sym.buffer());
    encoder.commit(std::move(sym));
  }
  std::cout << "---------------\n";
  {
    auto sym = ntc::symbol{256};
    std::copy(src      , src + 256, sym.buffer());
    sym.resize_buffer(512);
    std::copy(src + 256, src + 512, sym.buffer() + 256);
    encoder.commit(std::move(sym));
  }
  std::cout << "---------------\n";
  // Automatic symbol growing.
  {
    auto sym = ntc::auto_symbol{512};
    auto inserter = sym.back_inserter();
    std::copy(src, src + 256, inserter);
    std::copy(src, src + 256, inserter);
    std::copy(src, src + 256, inserter);
    encoder.commit(std::move(sym));
  }
  std::cout << "---------------\n";
  // Copy a buffer into the symbol.
  {
    auto sym = ntc::symbol{512, src};
    encoder.commit(std::move(sym));
  }
  std::cout << "---------------\n";

  std::cout << encoder.window_size() << '\n';

  std::array<char, 1> in;
  in[0] = static_cast<char>(ntc::detail::packet_type::ack);
  encoder.notify(in.data());
  in[0] = static_cast<char>(ntc::detail::packet_type::repair);
  encoder.notify(in.data());
  in[0] = static_cast<char>(ntc::detail::packet_type::source);
  encoder.notify(in.data());
  in[0] = 42;
  encoder.notify(in.data());

  return 0;
}
