#pragma once

#include <fstream>
#include <string>

namespace loss {

/*------------------------------------------------------------------------------------------------*/

class file
{
public:

  explicit file(std::ifstream&& file)
    : m_loss_file{std::move(file)}
  {}

  /// @return true if packet should be lost.
  bool
  operator()()
  noexcept
  {
    if (getline(m_loss_file, m_line))
    {
      return m_line[0] != '0';
    }
    else
    {
      // Don't drop packet if end of file.
      return false;
    }
  }

private:

  std::string m_line;
  std::ifstream m_loss_file;
};

/*------------------------------------------------------------------------------------------------*/

} // namespace loss
