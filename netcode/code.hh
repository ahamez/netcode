#pragma once

#include "netcode/detail/repair.hh"
#include "netcode/detail/source.hh"
#include "netcode/detail/source_list.hh"
#include "netcode/galois/field.hh"
#include "netcode/galois/multiply.hh"
#include "netcode/types.hh"

namespace ntc {

/*------------------------------------------------------------------------------------------------*/

/// @brief The component responsible for the encoding and decoding of @ref detail::repair.
class code final
{
public:

  /// @brief Constructor.
  code(unsigned int galois_field_size)
    : gf_{galois::field{galois_field_size}}
  {}

  /// @brief Fill a @ref detail::repair from a set of detail::source.
  /// @param repair The repair to fill.
  /// @param src_cit The beginning of the container of @ref detail::source.
  /// @param src_end The end of the container @ref detail::source. Must be different of @p src_cit.
  void
  encode( detail::repair& repair, detail::source_list::const_iterator src_cit
        , detail::source_list::const_iterator src_end)
  {
    assert(src_cit != src_end);

    // Resize the repair's symbol buffer to fit the first source symbol buffer.
    repair.buffer().resize(src_cit->buffer().size());

    repair.source_ids().emplace_back(src_cit->id());
    // Only multiply for the first source, no need to add with repair.
    multiply( gf_
            , src_cit->buffer().size()
            , src_cit->buffer().data(), repair.buffer().data()
            , mk_coefficient(repair.id(), src_cit->id()));

    // Then, for each remaining source, multiply it with a coefficient and add it with
    // current repair.
    for (++src_cit; src_cit != src_end; ++src_cit)
    {
      // The current repair's symbol buffer might be too small for the current source.
      if (src_cit->buffer().size() > repair.buffer().size())
      {
        repair.buffer().resize(src_cit->buffer().size());
      }

      repair.source_ids().emplace_back(src_cit->id());
      multiply_add( gf_
                  , src_cit->buffer().size()
                  , src_cit->buffer().data(), repair.buffer().data()
                  , mk_coefficient(repair.id(), src_cit->id()));
    }
  }

private:

  /// @brief Compute the coefficient for the repair being built.
  std::uint32_t
  mk_coefficient(std::uint32_t repair_id, std::uint32_t src)
  {
    return ((repair_id + 1) * (src + 1)) % gf_.size();
  }

  /// @brief An implementation of Galois fields.
  galois::field gf_;
};

/*------------------------------------------------------------------------------------------------*/

} // namespace ntc
