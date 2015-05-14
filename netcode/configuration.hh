#pragma once

#include <algorithm>
#include <cassert>
#include <chrono>
#include <limits> // numeric_limits

#include "netcode/code.hh"

namespace ntc {

/*------------------------------------------------------------------------------------------------*/

/// @brief The type to configure ntc::encoder and ntc::decoder.
class configuration
{
public:

  /// @brief Default constructor.
  /// @note Possible values for @p galois_field_size are: 4, 8, 16 and 32.
  /// @attention When 4 or 8, length of data handled to the library can be any value. However, for
  /// size 16 and 32, length must be a multiple of 2 and 4, respectively.
  configuration(std::uint8_t galois_field_size = 8)
    : galois_field_size_{galois_field_size}
    , code_type_{code::systematic}
    , rate_{5}
    , ack_frequency_{std::chrono::milliseconds{100}}
    , ack_nb_packets_{50}
    , window_{std::numeric_limits<std::size_t>::max()}
    , in_order_{true}
    , adaptative_{false}
  {
    assert( galois_field_size == 4 or galois_field_size == 8 or galois_field_size == 16
            or galois_field_size == 32);
  }

  /// @brief Get the Galois's field size.
  std::uint8_t
  galois_field_size()
  const noexcept
  {
    return galois_field_size_;
  }

  /// @brief Set the systematic/non-systematic mode of the code.
  void
  set_code_type(code c)
  noexcept
  {
    code_type_ = c;
  }

  /// @brief Get the systematic/non-systematic mode of the code.
  code
  code_type()
  const noexcept
  {
    return code_type_;
  }

  /// @brief Set how many sources are sent before a repair is generated.
  /// @note Must be greater than 0.
  void
  set_rate(std::size_t rate)
  noexcept
  {
    assert(rate > 0);
    rate_ = rate;
  }

  /// @brief Get how many sources are sent before a repair is generated.
  std::size_t
  rate()
  const noexcept
  {
    return rate_;
  }

  /// @brief Set the frequency at which ack will be sent from the decoder to the encoder.
  ///
  /// If 0, ack won't be sent automatically. The method decoder::send_ack() can still be called to
  /// force the sending of an ack.
  void
  set_ack_frequency(std::chrono::milliseconds f)
  noexcept
  {
    ack_frequency_ = f;
  }

  /// @brief Get the frequency at which ack will be sent from the decoder to the encoder.
  std::chrono::milliseconds
  ack_frequency()
  const noexcept
  {
    return ack_frequency_;
  }

  /// @brief Set how many packets to receive before an ack is sent from the decoder to the encoder.
  /// @note Must be greater than 0.
  void
  set_ack_nb_packets(std::uint16_t nb)
  noexcept
  {
    assert(nb > 0);
    ack_nb_packets_ = std::min(nb, static_cast<std::uint16_t>(128));
  }

  /// @brief Get how many packets to receive before an ack is sent from the decoder to the encoder.
  std::uint16_t
  ack_nb_packets()
  const noexcept
  {
    return ack_nb_packets_;
  }

  /// @brief Set the maximal permitted size of the encoder's window.
  /// @note Must be greater than 0.
  void
  set_window_size(std::size_t sz)
  noexcept
  {
    assert(sz > 0);
    window_ = sz;
  }

  /// @brief Get the maximal permitted size of the encoder's window.
  std::size_t
  window_size()
  const noexcept
  {
    return window_;
  }

  /// @brief Instruct decoder to give back data in order.
  void
  set_in_order(bool in_order)
  noexcept
  {
    in_order_ = in_order;
  }

  /// @brief Tell if data are given back in order.
  bool
  in_order()
  const noexcept
  {
    return in_order_;
  }

  /// @brief Set the adaptative mode of the code.
  void
  set_adaptative(bool adaptative)
  noexcept
  {
    adaptative_ = adaptative;
  }

  /// @brief Get the adaptative mode of the code.
  bool
  adaptative()
  const noexcept
  {
    return adaptative_;
  }

private:

  /// @brief The Galois field size.
  std::uint8_t galois_field_size_;

  /// @brief Tell if the code is systematic or not.
  code code_type_;

  /// @brief How many sources to send before a repair is generated.
  std::size_t rate_;

  /// @brief The frequency at which ack will be sent back from the decoder to the encoder.
  std::chrono::milliseconds ack_frequency_;

  /// @brief How many packets to receive before an ack is sent from the decoder to the encoder.
  std::uint16_t ack_nb_packets_;

  /// @brief The maximal number of sources to keep on the encoder side before discarding them.
  std::size_t window_;

  /// @brief Decoder gives back sources in order.
  bool in_order_;

  /// @brief Tell if the code is adaptative.
  bool adaptative_;
};

/*------------------------------------------------------------------------------------------------*/

} // namespace ntc
