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

  auto cit = sources.cbegin();
  const auto src_end = sources.cend();

  assert((reinterpret_cast<std::size_t>(cit->buffer().data()) % 16) == 0);

  // Resize the repair's symbol buffer to fit the first source symbol buffer.
  repair.buffer().resize(cit->user_size());

  // Add the current source id to the list of encoded sources by this repair.
  repair.source_ids().insert(repair.source_ids().end(), cit->id());

  // The coefficient for this repair and source.
  auto c = coefficient(gf_, repair.id(), cit->id());

  // Only multiply for the first source, no need to add with repair.
  gf_.multiply(cit->buffer().data(), repair.buffer().data(), cit->user_size(), c);

  // Initialize the user's size.
  repair.size() = gf_.multiply(c, static_cast<std::uint32_t>(cit->user_size()));

  // Then, for each remaining source, multiply it with a coefficient and add it with
  // current repair.
  for (++cit; cit != src_end; ++cit)
  {
    // The current repair's symbol buffer might be too small for the current source.
    if (cit->user_size() > repair.buffer().size())
    {
      repair.buffer().resize(cit->user_size());
    }

    // The coefficient for this repair and source.
    c = coefficient(gf_, repair.id(), cit->id());

    // Add the current source id to the list of encoded sources by this repair.
    repair.source_ids().insert(repair.source_ids().end(), cit->id());

    // Multiply and add for all following sources.
    gf_.multiply_add(cit->buffer().data(), repair.buffer().data(), cit->user_size(), c);

    // Finally, add the user size.
    repair.size() ^= gf_.multiply(c, static_cast<std::uint32_t>(cit->user_size()));
  }
}

/*------------------------------------------------------------------------------------------------*/

}} // namespace ntc::detail
