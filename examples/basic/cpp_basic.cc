#include <algorithm> // copy_n
#include <cassert>
#include <iostream>
#include <vector>

#include "netcode/decoder.hh"
#include "netcode/encoder.hh"

/*------------------------------------------------------------------------------------------------*/

// The handler of packets to send on networks, for both decoder and encoder.
struct packet_handler
{
  std::vector<char> buffer;

  packet_handler()
    : buffer()
  {}

  // This function is invoked repeatedly until the packet is complete.
  void
  operator()(const char* packet, std::size_t sz)
  {
    std::cout << "write " << sz << " bytes into packet\n";
    std::copy_n(packet, sz, std::back_inserter(buffer));
  }

  // This function is invoked when the packet received by the previous callback is complete.
  void
  operator()()
  {
    std::cout << "packet is ready to be sent!\n";
  }
};

/*------------------------------------------------------------------------------------------------*/

// The handler for data received or decoded, for the decoder only.
struct data_handler
{
  std::vector<char> buffer;

  data_handler()
    : buffer()
  {}

  // This function is invoked whenever a data is ready, that is a data which has been received or
  // decoded.
  void
  operator()(const char* data, std::size_t sz)
  {
    std::cout << sz << " bytes of data have been received!\n";
    std::copy_n(data, sz, std::back_inserter(buffer));
  }
};

/*------------------------------------------------------------------------------------------------*/

int main()
{
  try
  {
    // Create an encoder.
    ntc::encoder<packet_handler> enc{8, packet_handler{}};
    enc.set_window_size(32);

    // Create a decoder.
    ntc::decoder<packet_handler, data_handler> dec{8, true, packet_handler{}, data_handler{}};
    dec.set_ack_frequency(std::chrono::milliseconds{200});

    // Create an holder for some data with a size up to 1024 bytes.
    ntc::data data{1024};

    // Fill the data.
    data.buffer()[0] = 'a';
    data.buffer()[1] = 'b';
    data.buffer()[2] = 'c';

    // Don't forget to tell how many bytes were written.
    data.used_bytes() = 3;

    // Give this data to the encoder.
    // Be aware that the 'data' parameter will be invalid after this call. You can only call one
    // function on an invalid data: data::reset.
    enc(std::move(data));

    // The buffer of the packet handler for the encoder is now given to the decoder, as if it was
    // received from the network.
    dec(enc.packet_handler().buffer);

    // Now the context of the decoder's data handler contains the sent data.
    assert(dec.data_handler().buffer[0] == 'a');
    assert(dec.data_handler().buffer[1] == 'b');
    assert(dec.data_handler().buffer[2] == 'c');

    // Reset data: it's now legit to use it again.
    data.reset(1024);

    // We also clear buffers of handlers.
    enc.packet_handler().buffer.clear();
    dec.data_handler().buffer.clear();

    // Fill some more data.
    data.buffer()[0] = 'd';
    data.buffer()[1] = 'e';
    data.buffer()[2] = 'f';
    data.buffer()[3] = 'g';
    data.used_bytes() = 4;

    // Give this new data to the encoder.
    enc(std::move(data));

    // Again, The buffer of the packet handler for the encoder is now given to the decoder, as if it
    // was received from the network.
    dec(enc.packet_handler().buffer);

    // Now the context of the decoder's data handler contains the sent data.
    assert(dec.data_handler().buffer[0] == 'd');
    assert(dec.data_handler().buffer[1] == 'e');
    assert(dec.data_handler().buffer[2] == 'f');
    assert(dec.data_handler().buffer[3] == 'g');
  }
  catch(const std::exception& e)
  {
    // The netcode library uses exceptions to notify errors.
    std::cerr << "An error occurred: " << e.what() << '\n';
    return 1;
  }

  return 0;
}

/*------------------------------------------------------------------------------------------------*/