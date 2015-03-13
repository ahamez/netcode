#pragma once

#include <vector>

#include "netcode/detail/types.hh"

namespace ntc { namespace detail {

/*------------------------------------------------------------------------------------------------*/

/// @internal
class ack final
{
public:

  std::vector<id_type>&
  sources()
  noexcept
  {
    return sources_;
  }

private:

  id_type id_;
  std::vector<id_type> sources_;
};

/*------------------------------------------------------------------------------------------------*/

}} // namespace ntc::detail
