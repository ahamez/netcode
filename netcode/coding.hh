#pragma once

#include "netcode/detail/repair.hh"
#include "netcode/detail/source.hh"
#include "netcode/detail/source_list.hh"
#include "netcode/galois/field.hh"
#include "netcode/galois/multiply.hh"
#include "netcode/types.hh"

namespace ntc {

/*------------------------------------------------------------------------------------------------*/

class coding final
{
public:

  coding(galois::field&& gf, const coding_coefficient_generator& coefficient_generator)
    : gf_{std::move(gf)}, coeff_generator_{coefficient_generator}
  {}

  /// @brief
  /// @todo generate coefficients
  void
  operator()( detail::symbol_buffer_type& repair, detail::source_list::const_iterator src_cit
            , detail::source_list::const_iterator src_end)
  {
    // Resize the repair's symbol buffer to fit the first source symbol buffer.
    // Memory allocations will occur upon each symbol commit, until a maximal size is reached.
    repair.resize(src_cit->symbol_buffer().size());

    // Only multiply for the first source, no need to add with repair.
    galois::multiply( gf_
                    , src_cit->symbol_buffer().size()
                    , src_cit->symbol_buffer().data(), repair.data()
                    , 42 /* coeff to generate */);

    // Then, for each remaining source, multiply it with a coefficient and add it with
    // current repair.
    for (++src_cit; src_cit != src_end; ++src_end)
    {
      // The current repair's symbol buffer might be too small for the current source.
      if (src_cit->symbol_buffer().size() > repair.size())
      {
        repair.resize(src_cit->symbol_buffer().size());
      }
      galois::multiply_add( gf_
                          , src_cit->symbol_buffer().size()
                          , src_cit->symbol_buffer().data(), repair.data()
                          , 42 /* coeff to generate */);
    }
  }

private:

  galois::field gf_;
  coding_coefficient_generator coeff_generator_;
};

/*------------------------------------------------------------------------------------------------*/

} // namespace ntc
