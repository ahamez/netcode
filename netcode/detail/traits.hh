#pragma once

#include <type_traits>

#include "netcode/decoder_fwd.hh"
#include "netcode/encoder_fwd.hh"

namespace ntc { namespace detail {

/*------------------------------------------------------------------------------------------------*/

/// @internal
/// @brief Trait to detect if a type is ntc::encoder.
template <typename T>
struct is_encoder
{
  static constexpr auto value = false;
};

/// @internal
/// @brief Trait to detect if a type is ntc::encoder.
template <typename PacketHandler>
struct is_encoder<ntc::encoder<PacketHandler>>
{
  static constexpr auto value = true;
};

/// @internal
/// @brief Trait to detect if a type is ntc::decoder.
template <typename T>
struct is_decoder
{
  static constexpr auto value = false;
};

/// @internal
/// @brief Trait to detect if a type is ntc::encoder.
template <typename PacketHandler, typename DataHandler>
struct is_decoder<ntc::decoder<PacketHandler, DataHandler>>
{
  static constexpr auto value = true;
};

/*------------------------------------------------------------------------------------------------*/

/// @internal
/// @brief Dummy type for SFINAE
template<typename>
struct Void
{
  using type = void;
};

/// @internal
/// @brief Trait to detect if a type has begin()
template<typename T, typename SFINAE = void>
struct has_begin
{
  static constexpr auto value = false;
};

/// @internal
/// @brief Trait to detect if a type has begin()
template<typename T>
struct has_begin<T, typename Void<decltype(std::begin(std::declval<T&>()))>::type>
{
  static constexpr auto value = true;
};

/*------------------------------------------------------------------------------------------------*/

/// @internal
/// @brief Backport of the handy C++14 enable_if_t
template <bool Cond, typename Ret>
using enable_if_t = typename std::enable_if<Cond, Ret>::type;

/*------------------------------------------------------------------------------------------------*/

}} // namespace ntc::detail
