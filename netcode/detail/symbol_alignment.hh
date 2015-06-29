#pragma once

namespace ntc { namespace detail {

/*------------------------------------------------------------------------------------------------*/

/// @internal
/// @brief The boundary alignment requirement for a symbol
static constexpr auto symbol_alignment = 16ul;

/// @internal
/// @brief The headers length for both repair and source packets
static constexpr auto source_and_repair_headers = 7ul;

static_assert(source_and_repair_headers <= symbol_alignment, "");

/*------------------------------------------------------------------------------------------------*/

}} // namespace ntc::detail
