#pragma once

namespace ntc {

/*------------------------------------------------------------------------------------------------*/

struct symbol_start
{};

/*------------------------------------------------------------------------------------------------*/

struct symbol_end{};

/*------------------------------------------------------------------------------------------------*/

struct symbol_part
{
  const char* buffer_start;
  const char* buffer_end;
};

/*------------------------------------------------------------------------------------------------*/

} // namespace ntc
