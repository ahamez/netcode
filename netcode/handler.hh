#pragma once

#include <memory>

#include "netcode/galois/field.hh"
#include "netcode/types.hh"

namespace ntc {

/*------------------------------------------------------------------------------------------------*/

/// @internal
class handler_base
{
public:

  virtual ~handler_base(){}

  virtual void write(std::size_t nb, const char* data) = 0;
};

/*------------------------------------------------------------------------------------------------*/

/// @internal
template <typename Handler>
class handler_derived
  : public handler_base
{
public:

  template <typename H>
  handler_derived(H&& h)
    : handler_(std::forward<H>(h))
  {}

  void
  write(std::size_t nb, const char* data)
  {
    handler_.write(nb, data);
  }

private:

  Handler handler_;
};

/*------------------------------------------------------------------------------------------------*/

} // namespace ntc
