#pragma once

#include <cstdint>
#include <functional>
#include <vector>

namespace ntc {

/*------------------------------------------------------------------------------------------------*/

/// @brief Describe if a code is systematic or not.
enum class code_type {systematic, non_systematic};

/*------------------------------------------------------------------------------------------------*/

using id_type = std::uint32_t;

/*------------------------------------------------------------------------------------------------*/

using coding_coefficient_generator_t = std::function<id_type(id_type)>;

/*------------------------------------------------------------------------------------------------*/

/// @todo Ensure that the symbol is aligned on 16 bytes with a specific allocator.
using symbol_buffer_type = std::vector<char>;

/*------------------------------------------------------------------------------------------------*/

using write_fn = void (std::size_t /*bytes to write*/,const char* /*buffer to read from*/);

/*------------------------------------------------------------------------------------------------*/

} // namespace ntc
