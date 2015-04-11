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
// Encoder & Decoder stuff
/*------------------------------------------------------------------------------------------------*/

typedef void (*ntc_prepare_packet)(void*, const char* data, size_t sz);
typedef void (*ntc_send_packet)(void*);
typedef void (*ntc_read_data)(void*, const char* data, size_t sz);

typedef struct
{
  void* context;
  ntc_prepare_packet prepare_packet;
  ntc_send_packet send_packet;
} ntc_packet_handler;

typedef struct
{
  void* context;
  ntc_read_data read_data;
} ntc_data_handler;

#ifdef __cplusplus
struct c_packet_handler
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

struct c_data_handler
{
  ntc_data_handler handler;

  void
  operator()(const char* data, std::size_t sz)
  noexcept
  {
    handler.read_data(handler.context, data, sz);
  }
};
#endif

/*------------------------------------------------------------------------------------------------*/
// Encoder
/*------------------------------------------------------------------------------------------------*/

#ifdef __cplusplus
using ntc_encoder_t = ntc::encoder<c_packet_handler>;
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

void
ntc_encoder_notify_packet(ntc_encoder_t* enc, const char* packet);

void
ntc_encoder_end_repair(ntc_encoder_t* enc);

size_t
ntc_encoder_window(ntc_encoder_t* enc);

/*------------------------------------------------------------------------------------------------*/
// Decoder manipulation
/*------------------------------------------------------------------------------------------------*/

#ifdef __cplusplus
using ntc_decoder_t = ntc::decoder<c_packet_handler, c_data_handler>;
#else
typedef struct ntc_decoder_t ntc_decoder_t;
#endif

/*------------------------------------------------------------------------------------------------*/

ntc_decoder_t*
ntc_new_decoder(ntc_packet_handler packet_handler, ntc_data_handler data_handler);

void
ntc_delete_decoder(ntc_decoder_t* dec);

void
ntc_decoder_notify_packet(ntc_decoder_t* dec, const char* packet);

void
ntc_decoder_send_ack(ntc_decoder_t* dec);

/*------------------------------------------------------------------------------------------------*/

#ifdef __cplusplus
} // extern "C"
#endif
