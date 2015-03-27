#pragma once

#include "netcode/detail/coefficient.hh"
#include "netcode/detail/galois_field.hh"
#include "netcode/detail/multiple.hh"
#include "netcode/detail/repair.hh"
#include "netcode/detail/source.hh"
#include "netcode/detail/source_list.hh"

namespace ntc { namespace detail {

/*------------------------------------------------------------------------------------------------*/

/// @internal
/// @brief The component responsible for the encoding of detail::repair.
class encoder final
{
public:

  /// @brief Constructor.
  encoder(unsigned int galois_field_size)
    : gf_{detail::galois_field{galois_field_size}}
  {}

  /// @brief Fill a @ref detail::repair from a set of detail::source.
  /// @param repair The repair to fill.
  /// @param src_cit The beginning of the container of @ref detail::source.
  /// @param src_end The end of the container @ref detail::source. Must be different of @p src_cit.
  void
  operator()( repair& repair, source_list::const_iterator src_cit
            , source_list::const_iterator src_end)
  {
    assert(src_cit != src_end && "Empty source list");

    // Resize the repair's symbol buffer to fit the first source symbol buffer.
    repair.buffer().resize(make_multiple(src_cit->buffer().size(), 16));

    // Add the current source id to the list of encoded sources by this repair.
    repair.source_ids().emplace_back(src_cit->id());

    // The coefficient for this repair and source.
    auto coeff = coefficient(gf_, repair.id(), src_cit->id());

    // Only multiply for the first source, no need to add with repair.
    gf_.multiply( src_cit->buffer().data(), repair.buffer().data(), src_cit->buffer().size()
                , coeff);

    // Initialize the user's size.
    repair.size() = gf_.multiply(coeff, static_cast<std::uint32_t>(src_cit->user_size()));

    // Then, for each remaining source, multiply it with a coefficient and add it with
    // current repair.
    for (++src_cit; src_cit != src_end; ++src_cit)
    {
      // The current repair's symbol buffer might be too small for the current source.
      if (src_cit->buffer().size() > repair.buffer().size())
      {
        repair.buffer().resize(make_multiple(src_cit->buffer().size(), 16));
      }

      // The coefficient for this repair and source.
      coeff = coefficient(gf_, repair.id(), src_cit->id());

      // Add the current source id to the list of encoded sources by this repair.
      repair.source_ids().emplace_back(src_cit->id());

      // Multiply and add for all following sources.
      gf_.multiply_add( src_cit->buffer().data(), repair.buffer().data(), src_cit->buffer().size()
                      , coeff);

      // Finally, add the user size.
      repair.size() ^= gf_.multiply(coeff, static_cast<std::uint32_t>(src_cit->user_size()));
    }
  }

private:

  /// @brief The implementation of a Galois field.
  detail::galois_field gf_;
};

/*------------------------------------------------------------------------------------------------*/

}} // namespace ntc::detail
