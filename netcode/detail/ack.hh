#pragma once

#include <vector>

#include "netcode/detail/types.hh"
#include "netcode/types.hh"

namespace ntc { namespace detail {

/*------------------------------------------------------------------------------------------------*/

/// @internal
class ack final
{
public:

  /// @brief Can't copy-construct an ack.
  ack(const ack&) = delete;

  /// @brief Can't copy an ack.
  ack& operator=(const ack&) = delete;

  /// @brief Can move-construct an ack.
  ack(ack&&) = default;

  /// @brief Can move an ack.
  ack& operator=(ack&&) = default;

  /// @brief Constructor.
  ack(std::vector<id_type>&& source_ids)
    : source_ids_{std::move(source_ids)}
  {}

  /// @brief Get the list of acknowledged sources.
  const std::vector<id_type>&
  source_ids()
  const noexcept
  {
    return source_ids_;
  }

  /// @brief Get the list of acknowledged sources.
  std::vector<id_type>&
  source_ids()
  noexcept
  {
    return source_ids_;
  }

private:

  /// @brief The list of acknowledged sources.
  std::vector<id_type> source_ids_;
};

/*------------------------------------------------------------------------------------------------*/

}} // namespace ntc::detail
