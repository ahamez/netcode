#! /usr/bin/env python3

import argparse
import socket
import sys
import time

#------------------------------------------------------------------------------------------------------------#

def configure():

  parser = argparse.ArgumentParser()
  parser.add_argument('--packet-size', type=int, default=1024, help='bytes', metavar='value')
  parser.add_argument('--nb-packets', type=int, default=1024*1024, metavar='value')

  subparsers = parser.add_subparsers(dest='mode')

  client_parser = subparsers.add_parser('client', help='client mode')
  client_parser.add_argument('host', action='store')
  client_parser.add_argument('port', action='store', type=int)
  client_parser.add_argument('--period', type=int, default=1, help='ms', metavar='value')

  server_parser = subparsers.add_parser('server', help='server mode')
  server_parser.add_argument('--host', action='store', default='127.0.0.1')
  server_parser.add_argument('port', action='store', type=int)

  return parser.parse_args()

#------------------------------------------------------------------------------------------------------------#

def validate_packet(conf, id, data):

  if len(data) != conf.packet_size:
    print('\n#{} : invalid size'.format(id, len(data)))
    sys.exit(1)

  first_bytes = data[0:4]
  packet_id = int.from_bytes(first_bytes, byteorder='big')

  if packet_id != id:
    print('\n#{} : invalid id, got {}'.format(id, packet_id))
    sys.exit(1)

  # 1 -> skip 4 first bytes
  for i in range(1, int(conf.packet_size/4)):
    if data[(i * 4) : (i * 4) + 4] != first_bytes:
      print('\n#{} : invalid data, expected {}, got {}'.format(id, first_bytes, data[(i * 4) : (i * 4) + 4]))
      sys.exit(1)

def server(conf):

  sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
  sock.bind((conf.host, conf.port))

  current_id = 0
  nb_packets = 0

  t0 = time.monotonic()

  while True:
    data, _ = sock.recvfrom(4096)
    validate_packet(conf, current_id, data)
    current_id += 1
    nb_packets += 1

    t1 = time.monotonic()
    if (t1 - t0) >= 1:
      bandwidth = nb_packets * conf.packet_size / (t1 - t0)
      sys.stdout.write("Received {} packets @ ~{:.2f} Kbytes/s\r".format(current_id, bandwidth/1024))
      sys.stdout.flush()
      t0 = t1
      nb_packets = 0


#------------------------------------------------------------------------------------------------------------#

def make_packet(conf, id):
  return (id).to_bytes(4, byteorder='big') * int(conf.packet_size / 4)

def client(conf):
  host, port = conf.host, conf.port

  for i in range(0, conf.nb_packets):
    sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    sock.sendto(make_packet(conf, i), (host, port))
    time.sleep(conf.period / 1000)

#------------------------------------------------------------------------------------------------------------#

def main(conf):
  try:
    if conf.mode == 'client':
      client(conf)
    elif conf.mode == 'server':
      server(conf)
  except KeyboardInterrupt:
    print('Exiting...')
    sys.exit(0)

#------------------------------------------------------------------------------------------------------------#

if __name__ == '__main__':
  main(configure())
