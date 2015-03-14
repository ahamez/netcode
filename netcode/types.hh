#pragma once

#include <cstdint>
#include <functional>

namespace ntc {

/*------------------------------------------------------------------------------------------------*/

/// @brief Describe if a code is systematic or not.
enum class code_type {systematic, non_systematic};

/*------------------------------------------------------------------------------------------------*/

using id_type = std::uint32_t;

/*------------------------------------------------------------------------------------------------*/

using coding_coefficient_generator = std::function<id_type(id_type)>;

/*------------------------------------------------------------------------------------------------*/

} // namespace ntc
