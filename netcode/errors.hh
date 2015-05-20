#pragma once

#include <stdexcept>

namespace ntc {

/*------------------------------------------------------------------------------------------------*/

/// @brief Exception raised when a packet type could not have been decoded.
class packet_type_error
  : public std::domain_error
{
public:

  /// @internal
  /// @brief Constructor forwards all arguments to base class.
  template <typename... Args>
  packet_type_error(Args&&... args)
    : std::domain_error{std::forward<Args>(args)...}
  {}
};

/*------------------------------------------------------------------------------------------------*/

} // namespace ntc
