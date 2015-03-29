#pragma once

#include <functional> // function

namespace ntc {

/*------------------------------------------------------------------------------------------------*/

/// @brief
using handler = std::function<void(const char*, std::size_t)>;

/*------------------------------------------------------------------------------------------------*/

} // namespace ntc
