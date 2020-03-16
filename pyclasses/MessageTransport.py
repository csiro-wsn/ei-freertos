#!/usr/bin/env python3

import struct
import ctypes

from PacpMessage import PayloadType

class PacpTransport:
    def __init__(self):
        self._address = 0
        self._sequence = 0
        self._type = PayloadType.PAYLOAD_TDF3
        self._payload = b""

    @property
    def address(self):
        return self._address

    @property
    def sequence(self):
        return self._sequence

    @property
    def payload_type(self):
        return self._type

    @property
    def payload(self):
        return self._payload


class PacpTransportSerial(PacpTransport):
    SYNC = b"\xAA\x55"
    SYNC_LEN = len(SYNC)
    # SYNC_A SYNC_B (LEN_LSB LEN_MSB) (ADDR[0] ADDR[1] ADDR[2] ADDR[3] ADDR[4] ADDR[5]) SEQ_NO PAYLOAD_TYPE

    class SerialHeader(ctypes.LittleEndianStructure):
        _pack_ = 1
        _fields_ = [("sync", ctypes.c_char * 2), ("payload_len", ctypes.c_ushort), ("address", ctypes.c_ubyte * 6), ("sequence", ctypes.c_char), ("payload_type", ctypes.c_char)]
    HDR_LEN = ctypes.sizeof(SerialHeader)

    def __init__(self, address: int = 0, sequence: int = 0, payload_type: PayloadType = PayloadType.PAYLOAD_TDF3, payload: bytes = b""):
        self._address = address
        self._sequence = sequence
        self._type = payload_type
        self._payload = payload

    def __bytes__(self):
        addr_c = (ctypes.c_ubyte* 6)(*self._address.to_bytes(6, byteorder='little'))
        header_c = self.SerialHeader(self.SYNC, len(self._payload), addr_c, self._sequence, self._type.value)
        return bytes(header_c) + self._payload

    @classmethod
    def reconstruct(cls):
        consumed = True
        header_c = None
        buffer = b""
        # Consumes bytes via function.send(recv_byte)
        # Yields ( WasConsumed, GeneratedPacket )
        while True:
            # Take a byte
            recv_byte = yield
            consumed = True
            packet = None
            buffer += recv_byte
            if len(buffer) <= cls.SYNC_LEN:
                # If the sync doesn't match, discard any progress made
                if buffer[len(buffer)-1] != cls.SYNC[len(buffer)-1]:
                    buffer = b""
                    consumed = False
            elif len(buffer) < cls.HDR_LEN:
                pass
            elif len(buffer) == cls.HDR_LEN:
                header_c = cls.SerialHeader.from_buffer_copy(buffer)
            elif len(buffer) == cls.HDR_LEN + header_c.payload_len:
                payload = buffer[cls.HDR_LEN:]
                buffer = b""
                packet = cls(int.from_bytes(bytes(header_c.address), byteorder='little'), header_c.sequence, PayloadType(header_c.payload_type[0]), payload)
            yield (consumed, packet)

class PacpTransportSocket(PacpTransport):

    class SocketHeader(ctypes.LittleEndianStructure):
        _pack_ = 1
        _fields_ = [("payload_len", ctypes.c_ushort), ("address", ctypes.c_ubyte * 6), ("sequence", ctypes.c_char),  ("payload_type", ctypes.c_char)]
    HDR_LEN = ctypes.sizeof(SocketHeader)

    def __init__(self, address: int = 0, sequence: int = 0, payload_type: PayloadType = PayloadType.PAYLOAD_TDF3, payload: bytes = b""):
        self._address = address
        self._sequence = sequence
        self._type = payload_type
        self._payload = payload

    def __bytes__(self):
        addr_c = (ctypes.c_ubyte* 6)(*self._address.to_bytes(6, byteorder='little'))
        header_c = self.SocketHeader(len(self._payload), addr_c, self._sequence, self._type.value)
        return bytes(header_c) + self._payload

    def decode(self, buffer):
        self.header = self.SocketHeader.from_buffer_copy(buffer[:self.HDR_LEN])
        self.payload = buffer[self.HDR_LEN:]

    @classmethod
    def from_bytes(cls, byte_buffer):
        header_c = cls.SocketHeader.from_buffer_copy(byte_buffer[:cls.HDR_LEN])
        return cls(int.from_bytes(bytes(header_c.address), byteorder='little'), header_c.sequence, PayloadType(header_c.payload_type[0]), byte_buffer[cls.HDR_LEN:])

    @classmethod
    def receive_from(cls, socket):
        while True:
            header_raw = socket.recv(cls.HDR_LEN)
            if header_raw == b"":
                raise ConnectionResetError
            header_c = cls.SocketHeader.from_buffer_copy(header_raw)
            payload = socket.recv(header_c.payload_len)
            if header_c.payload_len != 0 and payload == b"":
                raise ConnectionResetError
            try:
                msg = cls(int.from_bytes(bytes(header_c.address), byteorder='little'), header_c.sequence, PayloadType(header_c.payload_type[0]), payload)
            except ValueError:
                continue
            break
        return msg

