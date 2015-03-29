#pragma once

namespace ntc { namespace detail {

/*------------------------------------------------------------------------------------------------*/

/// @internal
/// @brief Base class to erase the type of an handler.
struct handler_base
{
  /// @brief Can delete through this base class.
  virtual ~handler_base(){}

  /// @brief Called when a packet is ready to be sent.
  virtual void on_data(const char* data, std::size_t nb) = 0;

  /// @brief Called when a symbol is ready to be read.
  virtual void on_symbol(const char* data, std::size_t nb) = 0;
};

/*------------------------------------------------------------------------------------------------*/

/// @internal
/// @brief Hold the concrete type of a user's handler.
template <typename Handler>
struct handler_derived final
  : public handler_base
{
  /// @brief Construct with a copy or a moved user handler.
  template <typename H>
  handler_derived(H&& h)
    : handler_(std::forward<H>(h))
  {}

  void
  on_data(const char* data, std::size_t nb)
  override
  {
    handler_.on_data(data, nb);
  }

  void
  on_symbol(const char* data, std::size_t nb)
  override
  {
    handler_.on_symbol(data, nb);
  }

  /// @brief The user's handler.
  Handler handler_;
};

/*------------------------------------------------------------------------------------------------*/

}} // namespace ntc::detail
