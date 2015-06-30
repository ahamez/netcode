#include "netcode/packet.hh"

int main ()
{
  ///@ [example]
  // We start with a large buffer.
  ntc::packet pkt(4096);

  // Fill the packet with some data.
  pkt.assign({'a','b','c','d'});

  // We only used 4 bytes, we need to resize the packet before giving it to an encoder or a decoder.
  pkt.resize(4);
  ///@ [example]
}
