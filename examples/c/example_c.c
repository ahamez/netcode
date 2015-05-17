#include <assert.h>
#include <stdio.h>
#include <string.h>

#include <netcode/c/configuration.h>
#include <netcode/c/data.h>
#include <netcode/c/decoder.h>
#include <netcode/c/encoder.h>

/*------------------------------------------------------------------------------------------------*/

typedef struct
{
  size_t nb_read;
  char buffer[4096];
} context;

/*------------------------------------------------------------------------------------------------*/

void
prepare_packet(void* c, const char* packet, size_t sz)
{
  printf("write %lu bytes into packet\n", sz);
  context* cxt = (context*)c;
  memcpy(cxt->buffer + cxt->nb_read, packet, sz);
  cxt->nb_read += sz;
}

/*------------------------------------------------------------------------------------------------*/

void
send_packet(void* cxt)
{
  printf("packet is ready to be sent!\n");
  ((context*)cxt)->nb_read = 0;
}

/*------------------------------------------------------------------------------------------------*/

void
receive_data(void* cxt, const char* data, size_t sz)
{
  printf("data has been received!\n");
  memcpy(((context*)cxt)->buffer, data, sz);
}

/*------------------------------------------------------------------------------------------------*/

int main(int argc, char** argv)
{
  // Initialize several contexts for handlers.
  context encoder_cxt;
  encoder_cxt.nb_read = 0;
  memset(encoder_cxt.buffer, 'x', 4096);

  context decoder_cxt;
  decoder_cxt.nb_read = 0;
  memset(decoder_cxt.buffer, 'x', 4096);

  context data_cxt;
  data_cxt.nb_read = 0;
  memset(data_cxt.buffer, 'x', 4096);

  // A default configuration for encoder and decoder.
  ntc_configuration_t* conf = ntc_new_default_configuration();
  ntc_configuration_set_window_size(conf, 1024);

  // Create an encoder.
  ntc_packet_handler encoder_packet_handler = {&encoder_cxt, prepare_packet, send_packet};
  ntc_encoder_t* enc = ntc_new_encoder(conf, encoder_packet_handler);

  // Create a decoder.
  ntc_packet_handler decoder_packet_handler = {&decoder_cxt, prepare_packet, send_packet};
  ntc_data_handler decoder_data_handler = {&data_cxt, receive_data};
  ntc_decoder_t* dec = ntc_new_decoder(conf, decoder_packet_handler, decoder_data_handler);

  // Let's configure a little more the decoder.
  ntc_configuration_t* dec_conf = ntc_decoder_get_configuration(dec);
  ntc_configuration_set_ack_frequency(dec_conf, 200 /* ms */);

  // Prepare some data.
  ntc_data_t* data = ntc_new_data(1024);
  ntc_data_buffer(data)[0] = 'a';
  ntc_data_buffer(data)[1] = 'b';
  ntc_data_buffer(data)[2] = 'c';
  ntc_data_set_used_bytes(data, 3);

  // Give this data to the encoder.
  ntc_encoder_commit_data(enc, data);
  ntc_data_reset(data, 1024);

  // The context of the packet handler for the encoder is now given to the decoder, as if it was
  // received from the network.
  ntc_decoder_notify_packet(dec, encoder_cxt.buffer);

  // Now the context of the decoder's data handler contains the sent data.
  assert(data_cxt.buffer[0] == 'a');
  assert(data_cxt.buffer[1] == 'b');
  assert(data_cxt.buffer[2] == 'c');
  assert(data_cxt.buffer[3] == 'x');

  // Some cleanup.
  ntc_delete_configuration(conf);
  ntc_delete_data(data);
  ntc_delete_encoder(enc);
  ntc_delete_decoder(dec);

  return 0;
}
