#pragma once

#include <iosfwd>
#include <string>

namespace loss {

/*------------------------------------------------------------------------------------------------*/

class stream
{
public:

  explicit stream(std::istream& s)
    : m_loss_stream{s}
  {}

  /// @return true if packet should be lost.
  bool
  operator()()
  noexcept
  {
    if (getline(m_loss_stream, m_line))
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
  std::istream& m_loss_stream;
};

/*------------------------------------------------------------------------------------------------*/

} // namespace loss
