#include <exception>
#include <new> // nothrow

#include "netcode/c/configuration.h"

/*------------------------------------------------------------------------------------------------*/

ntc_configuration_t*
ntc_new_configuration(uint8_t galois_field_size)
noexcept
{
  return new (std::nothrow) ntc_configuration_t{galois_field_size};
}

/*------------------------------------------------------------------------------------------------*/

ntc_configuration_t*
ntc_new_default_configuration()
noexcept
{
  return new (std::nothrow) ntc_configuration_t{8};
}

/*------------------------------------------------------------------------------------------------*/

void
ntc_delete_configuration(ntc_configuration_t* conf)
noexcept
{
  delete conf;
}

/*------------------------------------------------------------------------------------------------*/

void
ntc_configuration_set_code_type(ntc_configuration_t* conf, ntc_code_type code_type)
noexcept
{
  switch (code_type)
  {
    case ntc_systematic:
      conf->set_code_type(ntc::code::systematic);
      break;

    case ntc_non_systematic:
    default:
      conf->set_code_type(ntc::code::non_systematic);
  }
}

/*------------------------------------------------------------------------------------------------*/

void
ntc_configuration_set_rate(ntc_configuration_t* conf, size_t rate)
noexcept
{
  conf->set_rate(rate);
}

/*------------------------------------------------------------------------------------------------*/

void
ntc_configuration_set_ack_frequency(ntc_configuration_t* conf, size_t frequency)
noexcept
{
  conf->set_ack_frequency(std::chrono::milliseconds{frequency});
}

/*------------------------------------------------------------------------------------------------*/

void
ntc_configuration_set_ack_nb_packets(ntc_configuration_t* conf, uint16_t nb)
noexcept
{
  conf->set_ack_nb_packets(nb);
}

/*------------------------------------------------------------------------------------------------*/

void
ntc_configuration_set_window_size(ntc_configuration_t* conf, size_t window)
noexcept
{
  conf->set_window_size(window);
}

/*------------------------------------------------------------------------------------------------*/

void
ntc_configuration_set_in_order(ntc_configuration_t* conf, bool in_order)
noexcept
{
  conf->set_in_order(in_order);
}

/*------------------------------------------------------------------------------------------------*/

void
ntc_configuration_set_adaptive(ntc_configuration_t* conf, bool adaptive)
noexcept
{
  conf->set_adaptive(adaptive);
}

/*------------------------------------------------------------------------------------------------*/
