#pragma once

#include "netcode/detail/buffer.hh"

namespace ntc { namespace detail {

/*------------------------------------------------------------------------------------------------*/

/// @internal
/// @brief A source packet holding a user's symbol
class source final
{
public:

  /// @brief Can't copy-construct a source
  source(const source&) = delete;

  /// @brief Can't copy a source
  source& operator=(const source&) = delete;

  /// @brief Can move-construct a source
  source(source&&) = default;

  /// @brief Can move a source
  source& operator=(source&&) = default;

  /// @brief Constructor
  source(std::uint32_t id, byte_buffer&& buf)
    : m_id{id}
    , m_symbol_buffer{std::move(buf)}
  {}

  /// @brief Get this source's identifier
  std::uint32_t
  id()
  const noexcept
  {
    return m_id;
  }

  /// @brief Get the bytes of the symbol
  const byte_buffer&
  symbol()
  const noexcept
  {
    return m_symbol_buffer;
  }

  /// @brief Get the bytes of the symbol
  byte_buffer&
  symbol()
  noexcept
  {
    return m_symbol_buffer;
  }

  /// @brief Get the number of bytes in the user's symbol
  /// @note This method is just a helper to avoid the static_cast eveywhere the size is needed
  std::uint16_t
  size()
  const noexcept
  {
    return static_cast<std::uint16_t>(m_symbol_buffer.size());
  }

private:

  /// @brief This source's unique identifier
  std::uint32_t m_id;

  /// @brief This source's symbol
  byte_buffer m_symbol_buffer;
};

/*------------------------------------------------------------------------------------------------*/

}} // namespace ntc::detail
