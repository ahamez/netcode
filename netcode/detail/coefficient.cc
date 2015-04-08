#include "netcode/detail/coefficient.hh"

namespace ntc { namespace detail {

/*------------------------------------------------------------------------------------------------*/

std::uint32_t
coefficient(const galois_field& gf, std::uint32_t repair_id, std::uint32_t src_id)
noexcept
{
  return ((repair_id + 1) * (src_id + 1)) % (1 << gf.size());
}

/*------------------------------------------------------------------------------------------------*/

}} // namespace ntc::detail
