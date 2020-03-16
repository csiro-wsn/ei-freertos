#!/usr/bin/env python3

import socket
import struct

from MessageTransport import PacpTransportSocket, PacpTransportSocket
from PacpMessage import PacpPacket, PayloadType

class ListenClient:

    def __init__(self, host="localhost", port=9001 ):
        self.host = host
        self.port = port
        self.socket = None

    def connect(self):
        self.socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)

        try:
            self.socket.settimeout(15)
            self.socket.connect((self.host, self.port))
        except (ConnectionRefusedError, socket.error) as err:
            print("Cant connect to (%s,%s): %s" % (self.host, self.port , str(err)))
            raise

    def close(self):
        try:
            self.socket.shutdown(socket.SHUT_RDWR)
            self.socket.close()
        except (ConnectionRefusedError, socket.error):
            pass
    
    def write(self, packet):
        self.socket.sendall( bytes(packet) )

    def read(self, timeout=None):
        self.socket.settimeout(timeout)
        socket_packet = PacpTransportSocket.receive_from(self.socket)
        packet = PacpPacket.from_bytes(socket_packet.payload_type, socket_packet.address, socket_packet.payload)
        return packet

    def __repr__(self):
        return "{:s}:{:d}".format(self.host, self.port)


if __name__ == '__main__':
    listener = ListenClient(port=9001)
    import time
    listener.connect()
    listener.read()
    listener.close()
