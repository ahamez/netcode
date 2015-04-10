#include <stdio.h>

#include <netcode/cnetcode.h>

void
prepare_packet(void* cxt, const char* data, size_t sz)
{
  printf("write %lu bytes into packet\n", sz);
}

void
send_packet(void* cxt)
{
  printf("packet is ready!\n");
}

typedef struct
{

} context;

int main(int argc, char** argv)
{
  context cxt;
  ntc_packet_handler handler = {(void*)&cxt, prepare_packet, send_packet};
  ntc_encoder_t* enc = ntc_new_encoder(handler);

  ntc_data_t* data = ntc_new_data(1024);
  ntc_data_used_bytes(data, 20);

  ntc_encoder_commit_data(enc, data);
  ntc_data_reset(data, 1024);

  ntc_delete_data(data);
  ntc_delete_encoder(enc);

  return 0;
}