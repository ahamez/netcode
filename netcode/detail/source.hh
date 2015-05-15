#pragma once

#include "netcode/detail/buffer.hh"

namespace ntc { namespace detail {

/*------------------------------------------------------------------------------------------------*/

/// @internal
/// @brief A source packet holding a user's symbol.
class source final
{
public:

  /// @brief Can't copy-construct a source.
  source(const source&) = delete;

  /// @brief Can't copy a source.
  source& operator=(const source&) = delete;

  /// @brief Can move-construct a source.
  source(source&&) = default;

  /// @brief Can move a source.
  source& operator=(source&&) = default;

  /// @brief Constructor.
  source(std::uint32_t id, byte_buffer&& buf, std::uint16_t user_size)
    : m_id{id}
    , m_symbol_buffer{std::move(buf)}
    , m_user_size{user_size}
  {}

  /// @brief Get this source's identifier.
  std::uint32_t
  id()
  const noexcept
  {
    return m_id;
  }

  /// @brief Get the bytes of the symbol.
  const byte_buffer&
  buffer()
  const noexcept
  {
    return m_symbol_buffer;
  }

  /// @brief Get the bytes of the symbol.
  byte_buffer&
  buffer()
  noexcept
  {
    return m_symbol_buffer;
  }

  /// @brief Get the number of bytes really used by the user's symbol.
  std::uint16_t
  user_size()
  const noexcept
  {
    return m_user_size;
  }

private:

  /// @brief This source's unique identifier.
  std::uint32_t m_id;

  /// @brief This source's symbol.
  byte_buffer m_symbol_buffer;

  /// @brief The number of bytes really used by the user's symbol in the buffer.
  std::uint16_t m_user_size;
};

/*------------------------------------------------------------------------------------------------*/

}} // namespace ntc::detail
