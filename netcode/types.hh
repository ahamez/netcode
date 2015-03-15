#pragma once

#include <cstdint>
#include <functional>

namespace ntc {

/*------------------------------------------------------------------------------------------------*/

/// @brief Describe if a code is systematic or not.
enum class code_type {systematic, non_systematic};

/*------------------------------------------------------------------------------------------------*/

using coding_coefficient_generator = std::function<std::uint32_t(std::uint32_t)>;

/*------------------------------------------------------------------------------------------------*/

} // namespace ntc
