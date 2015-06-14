#include <assert.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>

#include <netcode/c/data.h>
#include <netcode/c/decoder.h>
#include <netcode/c/encoder.h>

/*------------------------------------------------------------------------------------------------*/

// A context to save informations needed by callbacks (like a socket or a buffer).
typedef struct
{
  size_t nb_read;
  char buffer[4096];
} context;

/*------------------------------------------------------------------------------------------------*/

// This callback is invoked repeatedly (by the encoder or the decoder) until the packet is complete.
void
prepare_packet(void* c, const char* packet, size_t sz)
{
  printf("write %lu bytes into packet\n", sz);
  context* cxt = (context*)c;
  memcpy(cxt->buffer + cxt->nb_read, packet, sz);
  cxt->nb_read += sz;
}

/*------------------------------------------------------------------------------------------------*/

// This callback is invoked (by the encoder or the decoder) when the packet received by the callback
// prepare_packet is complete.
void
send_packet(void* cxt)
{
  printf("packet is ready to be sent!\n");
  ((context*)cxt)->nb_read = 0;
}

/*------------------------------------------------------------------------------------------------*/

// This callback is invoked (by the decoder) whenever a data is ready, that is a data which has been
// received or decoded.
void
receive_data(void* cxt, const char* data, size_t sz)
{
  printf("%lu bytes of data have been received!\n", sz);
  memcpy(((context*)cxt)->buffer, data, sz);
  ((context*)cxt)->nb_read = sz;
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

  // Prepare the handler of packets (given by the encoder) which are ready to be sent.
  ntc_packet_handler encoder_packet_handler = {&encoder_cxt, prepare_packet, send_packet};

  // Create an encoder.
  ntc_encoder_t* enc = ntc_new_encoder(8, encoder_packet_handler);
  ntc_encoder_set_window_size(enc, 32);

  // Prepare the handler of packets (given by the decoder) which are ready to be sent.
  ntc_packet_handler decoder_packet_handler = {&decoder_cxt, prepare_packet, send_packet};

  // Prepare the handler of data (given by the decoder) which are ready to be given to the user.
  ntc_data_handler decoder_data_handler = {&data_cxt, receive_data};

  // Create a decoder.
  ntc_decoder_t* dec = ntc_new_decoder(8, true, decoder_packet_handler, decoder_data_handler);
  ntc_decoder_set_ack_frequency(dec, 200 /* ms */);

  // Create an holder for some data with a size up to 1024 bytes.
  ntc_data_t* data = ntc_new_data(1024);

  // Fill the data.
  ntc_data_buffer(data)[0] = 'a';
  ntc_data_buffer(data)[1] = 'b';
  ntc_data_buffer(data)[2] = 'c';

  // Don't forget to tell how many bytes were written.
  ntc_data_set_used_bytes(data, 3);

  // Filled by the library if an error happen.
  ntc_error error;

  // Give this data to the encoder.
  // Be aware that the 'data' parameter will be invalid after this call. You can only call two
  // functions on an invalid data: ntc_data_reset or ntc_delete_data.
  ntc_encoder_add_data(enc, data, &error);

  // Check if the previous operation succeeded.
  if (error.type != ntc_no_error)
  {
    goto handle_error;
  }

  // The context of the packet handler for the encoder is now given to the decoder, as if it was
  // received from the network.
  ntc_decoder_add_packet(dec, encoder_cxt.buffer, 1, &error);

  // Check if the previous operation succeeded.
  if (error.type != ntc_no_error)
  {
    goto handle_error;
  }

  // Now the context of the decoder's data handler contains the sent data.
  assert(data_cxt.nb_read == 3);
  assert(data_cxt.buffer[0] == 'a');
  assert(data_cxt.buffer[1] == 'b');
  assert(data_cxt.buffer[2] == 'c');

  // Reset data: it's now legit to use it again.
  ntc_data_reset(data, 1024, &error);

  // Check if the previous operation succeeded.
  if (error.type != ntc_no_error)
  {
    goto handle_error;
  }

  // Fill some more data.
  ntc_data_buffer(data)[0] = 'd';
  ntc_data_buffer(data)[1] = 'e';
  ntc_data_buffer(data)[2] = 'f';
  ntc_data_buffer(data)[3] = 'g';
  ntc_data_set_used_bytes(data, 4);

  // Give this new data to the encoder.
  ntc_encoder_add_data(enc, data, &error);

  // Check if the previous operation succeeded.
  if (error.type != ntc_no_error)
  {
    goto handle_error;
  }

  // Again, the context of the packet handler for the encoder is now given to the decoder, as if it
  //  was received from the network.
  ntc_decoder_add_packet(dec, encoder_cxt.buffer, 4096, &error);

  // Check if the previous operation succeeded.
  if (error.type != ntc_no_error)
  {
    goto handle_error;
  }

  // Now the context of the decoder's data handler contains the sent data.
  assert(data_cxt.nb_read == 4);
  assert(data_cxt.buffer[0] == 'd');
  assert(data_cxt.buffer[1] == 'e');
  assert(data_cxt.buffer[2] == 'f');
  assert(data_cxt.buffer[3] == 'g');

  // Cleanup memory.
  ntc_delete_data(data);
  ntc_delete_encoder(enc);
  ntc_delete_decoder(dec);

  return 0;

handle_error:
  printf("An error occurred %s\n", error.message ? error.message : "");
  return 1;
}

/*------------------------------------------------------------------------------------------------*/
