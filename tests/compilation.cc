#include <array>
#include <iostream>

#include "netcode/galois/multiply.hh"
#include "netcode/encoder.hh"

int main()
{
  auto field = galois::field{8};

  std::array<char[256], 2> src;
//  char dst[256];
//
//  std::fill(dst, dst+256, 0);
//  std::fill(src[0], src[0]+256, 23);
//  std::fill(src[1], src[1]+256, 12);
//
//  for (auto c : dst)
//  {
//    std::cout << +c << ',';
//  }
//  std::cout << "\n\n";
//
//  galois::multiply_add(field, begin(src), end(src), dst, 1, 250);
//
//  for (auto c : dst)
//  {
//    std::cout << +c << ',';
//  }
//  std::cout << "\n\n";
//
//  galois::multiply_add(field, src[0], dst, 2, 256);
//
//  for (auto c : dst)
//  {
//    std::cout << +c << ',';
//  }
//  std::cout << '\n';

  auto coding = ntc::coding{galois::field{8}, [](ntc::id_type x){return x;}};
  auto encoder = ntc::encoder{std::move(coding)};

  // Copy to inner buffer
  encoder << ntc::symbol_start{};
  encoder << ntc::symbol_part{src[0], src[0] + 256}
          << ntc::symbol_part{src[1], src[1] + 256};
  encoder << ntc::symbol_end{};

  // Directly write to inner buffer
  encoder.buffer_size() = 2048;
  encoder.start_symbol();

  std::copy(src[0], src[0] + 256, encoder.buffer());
  std::copy(src[1], src[1] + 256, encoder.buffer());
  // vs
  encoder.write(src[0], src[0] + 256);
  encoder.write(src[1], src[1] + 256);

  encoder.end_symbol();

  return 0;
}
