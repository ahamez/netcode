#include <cstring>

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
  context* cxt = (context*)c;
  memcpy(cxt->buffer + cxt->nb_read, packet, sz);
  cxt->nb_read += sz;
}

/*------------------------------------------------------------------------------------------------*/

void
send_packet(void* cxt)
{
  ((context*)cxt)->nb_read = 0;
}

/*------------------------------------------------------------------------------------------------*/

void
receive_data(void* cxt, const char* data, size_t sz)
{
  memcpy(((context*)cxt)->buffer, data, sz);
  ((context*)cxt)->nb_read = sz;
}

/*------------------------------------------------------------------------------------------------*/
