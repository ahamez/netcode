#include "netcode/detail/encoder.hh"

namespace ntc { namespace detail {

/*------------------------------------------------------------------------------------------------*/

encoder::encoder(std::uint8_t galois_field_size)
  : m_gf{galois_field_size}
{}

/*------------------------------------------------------------------------------------------------*/

void
encoder::operator()(encoder_repair& repair, source_list& sources)
{
  assert(sources.size() && "Empty source list");

  auto cit = sources.cbegin();
  const auto src_end = sources.cend();

  assert((reinterpret_cast<std::size_t>(cit->symbol().data()) % 16) == 0);

  // Resize the repair's symbol buffer to fit the first source symbol buffer.
  repair.symbol().resize(cit->size());

  // Add the current source id to the list of encoded sources by this repair.
  repair.source_ids().insert(repair.source_ids().end(), cit->id());

  // The coefficient for this repair and source.
  auto c = m_gf.coefficient(repair.id(), cit->id());

  // Only multiply for the first source, no need to add with repair.
  m_gf.multiply(cit->symbol().data(), repair.symbol().data(), cit->size(), c);

  // Initialize the user's size.
  repair.encoded_size() = m_gf.multiply_size(cit->size(), c);

  // Then, for each remaining source, multiply it with a coefficient and add it with
  // current repair.
  for (++cit; cit != src_end; ++cit)
  {
    // The current repair's symbol buffer might be too small for the current source.
    if (cit->size() > repair.symbol().size())
    {
      repair.symbol().resize(cit->size());
    }

    // The coefficient for this repair and source.
    c = m_gf.coefficient(repair.id(), cit->id());

    // Add the current source id to the list of encoded sources by this repair.
    repair.source_ids().insert(repair.source_ids().end(), cit->id());

    // Multiply and add for all following sources.
    m_gf.multiply_add(cit->symbol().data(), repair.symbol().data(), cit->size(), c);

    // Finally, add the user size.
    // Cast is necessary to inhibit conversion warning as xor implicitly convert to a signed value.
    repair.encoded_size()
      = static_cast<std::uint16_t>(m_gf.multiply_size(cit->size(), c) ^ repair.encoded_size());
  }
}

/*------------------------------------------------------------------------------------------------*/

}} // namespace ntc::detail
