#include "netcode/detail/coefficient.hh"
#include "netcode/detail/encoder.hh"
#include "netcode/detail/multiple.hh"

namespace ntc { namespace detail {

/*------------------------------------------------------------------------------------------------*/

encoder::encoder(std::size_t galois_field_size)
  : gf_{galois_field_size}
{}

/*------------------------------------------------------------------------------------------------*/

void
encoder::operator()(repair& repair, source_list& sources)
{
  assert(sources.size() && "Empty source list");

  auto src_cit = sources.cbegin();
  const auto src_end = sources.cend();

  assert((reinterpret_cast<std::size_t>(src_cit->buffer().data()) % 16) == 0);

  auto user_sz16 = multiple(src_cit->user_size(), 16);
  assert(user_sz16 <= src_cit->buffer().size());

  // Resize the repair's symbol buffer to fit the first source symbol buffer.
  repair.buffer().resize(user_sz16);

  // Add the current source id to the list of encoded sources by this repair.
  repair.source_ids().insert(repair.source_ids().end(), src_cit->id());

  // The coefficient for this repair and source.
  auto coeff = coefficient(gf_, repair.id(), src_cit->id());

  // Only multiply for the first source, no need to add with repair.
  gf_.multiply(src_cit->buffer().data(), repair.buffer().data(), user_sz16, coeff);

  // Initialize the user's size.
  repair.size() = gf_.multiply(coeff, static_cast<std::uint32_t>(src_cit->user_size()));

  // Then, for each remaining source, multiply it with a coefficient and add it with
  // current repair.
  for (++src_cit; src_cit != src_end; ++src_cit)
  {
    auto user_sz16 = multiple(src_cit->user_size(), 16);
    assert(user_sz16 <= src_cit->buffer().size());

    // The current repair's symbol buffer might be too small for the current source.
    if (user_sz16 > repair.buffer().size())
    {
      repair.buffer().resize(user_sz16);
    }

    // The coefficient for this repair and source.
    coeff = coefficient(gf_, repair.id(), src_cit->id());

    // Add the current source id to the list of encoded sources by this repair.
    repair.source_ids().insert(repair.source_ids().end(), src_cit->id());

    // Multiply and add for all following sources.
    gf_.multiply_add(src_cit->buffer().data(), repair.buffer().data(), user_sz16, coeff);

    // Finally, add the user size.
    repair.size() ^= gf_.multiply(coeff, static_cast<std::uint32_t>(src_cit->user_size()));
  }
}

/*------------------------------------------------------------------------------------------------*/

}} // namespace ntc::detail
