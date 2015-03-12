#pragma once

#include "netcode/galois/field.hh"
#include "netcode/types.hh"

namespace ntc {

/*------------------------------------------------------------------------------------------------*/

class coding
{
public:

  coding(galois::field&& gf, const coding_coefficient_generator_t& coefficient_generator)
    : gf_{std::move(gf)}, coeff_generator_{coefficient_generator}
  {}

private:

  galois::field gf_;
  coding_coefficient_generator_t coeff_generator_;
};

/*------------------------------------------------------------------------------------------------*/

} // namespace ntc
