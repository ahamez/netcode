#pragma once

#ifdef __cplusplus
#include <netcode/data.hh>
#include <netcode/decoder.hh>
#include <netcode/encoder.hh>
#endif

#ifdef __cplusplus
extern "C" {
#endif

/*------------------------------------------------------------------------------------------------*/
// Data
/*------------------------------------------------------------------------------------------------*/

#ifdef __cplusplus
using ntc_data_t = ntc::data;
#else
typedef struct ntc_data_t ntc_data_t;
#endif

/*------------------------------------------------------------------------------------------------*/

ntc_data_t*
ntc_new_data(size_t sz);

ntc_data_t*
ntc_new_data_copy(const char* src, size_t sz);

void
ntc_delete_data(ntc_data_t* d);

char*
ntc_data_buffer(ntc_data_t* d);

void
ntc_data_used_bytes(ntc_data_t* d, size_t sz);

void
ntc_data_reset(ntc_data_t* d, size_t sz);

/*------------------------------------------------------------------------------------------------*/
// Encoder
/*------------------------------------------------------------------------------------------------*/

typedef void (*ntc_prepare_packet_handler)(void*, const char* data, size_t sz);
typedef void (*ntc_send_packet_handler)(void*);

typedef struct
{
  void* context;
  ntc_prepare_packet_handler prepare_packet;
  ntc_send_packet_handler send_packet;
} ntc_packet_handler;

#ifdef __cplusplus
namespace ntc { namespace detail {

struct c_handler
{
  ntc_packet_handler handler;

  void
  operator()(const char* data, std::size_t sz)
  noexcept
  {
    handler.prepare_packet(handler.context, data, sz);
  }

  void
  operator()()
  noexcept
  {
    handler.send_packet(handler.context);
  }
};

}} // namespace ntc::detail

using ntc_encoder_t = ntc::encoder<ntc::detail::c_handler>;
#else
typedef struct ntc_encoder_t ntc_encoder_t;
#endif

/*------------------------------------------------------------------------------------------------*/

ntc_encoder_t*
ntc_new_encoder(ntc_packet_handler handler);

void
ntc_delete_encoder(ntc_encoder_t* enc);

void
ntc_encoder_commit_data(ntc_encoder_t* enc, ntc_data_t* data);

/*------------------------------------------------------------------------------------------------*/
// Decoder manipulation
/*------------------------------------------------------------------------------------------------*/


/*------------------------------------------------------------------------------------------------*/

#ifdef __cplusplus
} // extern "C"
#endif
