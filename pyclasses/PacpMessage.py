#!/usr/bin/env python3

from Crypto.Cipher import AES
from enum import Enum, unique
from typing import List
import ctypes

class DecryptionError(Exception):
    pass

@unique
class MessageInterface(Enum):
    COMMS_INTERFACE_SERIAL = 0
    COMMS_INTERFACE_BLUETOOTH = 1
    COMMS_INTERFACE_GATT = 2
    COMMS_INTERFACE_LORA = 3
    COMMS_INTERFACE_LORAWAN = 4

    def __str__(self):
        to_string = {
            self.COMMS_INTERFACE_SERIAL : "serial",
            self.COMMS_INTERFACE_BLUETOOTH : "bt_adv",
            self.COMMS_INTERFACE_GATT : "bt_gatt",
            self.COMMS_INTERFACE_LORA : "LoRa",
            self.COMMS_INTERFACE_LORAWAN : "LoRaWAN",
        }
        return to_string[self]

@unique
class PayloadType(Enum):
    PAYLOAD_TDF3 = 0
    PAYLOAD_OTI = 1
    PAYLOAD_VTI = 2
    PAYLOAD_RPC = 3
    PAYLOAD_RPC_RSP = 4
    PAYLOAD_INCOMING = 5
    PAYLOAD_OUTGOING = 6

    PAYLOAD_ENC_TDF3 = 32
    PAYLOAD_ENC_OTI = 33
    PAYLOAD_ENC_VTI = 34
    PAYLOAD_ENC_RPC = 35
    PAYLOAD_ENC_RPC_RSP = 36

    PAYLOAD_CUSTOM = 255

    def __bytes__(self):
        return self.value.to_bytes(1, byteorder='little')

class Hop:

    class OutgoingRouteInfo(ctypes.LittleEndianStructure):
        _pack_ = 1
        _fields_ = [("address", 6 * ctypes.c_uint8), ("interface_channel", ctypes.c_uint8)]
    OUTGOING_SIZE = ctypes.sizeof(OutgoingRouteInfo)

    class IncomingRouteInfo(ctypes.LittleEndianStructure):
        _pack_ = 1
        _fields_ = [("address", 6 * ctypes.c_uint8), ("interface_channel", ctypes.c_uint8), ("packet_age", ctypes.c_uint16), ("sequence", ctypes.c_uint8), ("rssi", ctypes.c_uint8)]
    INCOMING_SIZE = ctypes.sizeof(IncomingRouteInfo)

    def __init__(self, address: int, interface: MessageInterface, channel: str):
        self._address = address
        self._address_fmt = "00:00"
        self._interface = interface
        self._channel = channel
        self.__rssi = 0
        self._packet_age = 0
        self._sequence_number = 0
        self._metadata = False

    def apply_metadata(self, packet_age: int, sequence_number: int, rssi: int):
        self._packet_age = packet_age
        self._sequence_number = sequence_number
        self._rssi = rssi
        self._metadata = True

    def __bytes__(self):
        interface_channel = (self._interface.value << 4) | (0)
        address = (ctypes.c_ubyte* 6)(*self._address.to_bytes(6, byteorder='little'))
        if self._metadata:
            route_c =  self.IncomingRouteInfo( address, interface_channel, self._packet_age, self._sequence_number, 30 - self._rssi)
        else:
            route_c =  self.OutgoingRouteInfo( address, interface_channel)
        return bytes(route_c)

    def update_address(self, address: int):
        self._address = address

    @property
    def address(self):
        return self._address

    @property
    def address_fmt(self):
        return "{:02X}:{:02X}:{:02X}:{:02X}:{:02X}:{:02X}".format(*bytearray(self._address.to_bytes(6, byteorder='big')))

    @property
    def interface(self):
        return self._interface

    @property
    def rssi(self):
        return self._rssi

    @classmethod
    def from_bytes(cls, byte_buffer : bytes):
        hop_c = cls.IncomingRouteInfo.from_buffer_copy(byte_buffer)
        address = int.from_bytes(bytes(hop_c.address), byteorder='little')
        try:
            route = cls(address, MessageInterface(hop_c.interface_channel >> 4), "default" )
        except ValueError as e:
            print("Bad Packet: " + " ".join([hex(b) for b in byte_buffer]))
            raise e
        route.apply_metadata(hop_c.packet_age, hop_c.sequence, 30 - hop_c.rssi)
        return route

class PacpPacket:

    class Payload:
        def __init__(self, payload_type: PayloadType, last_hop: Hop, payload: bytes):
            self._type = payload_type
            self._hop = last_hop
            self._payload = payload

        def __bytes__(self):
            payload =  bytes(self._type) + bytes(self._hop) + self._payload
            return (1 + len(payload)).to_bytes(1, byteorder='little') + payload

    PayloadList = List[Payload]
    HopList = List[Hop]

    def __init__(self, common_route: HopList, payloads: PayloadList):
        self._payloads = payloads
        self._common_route = common_route

    def __bytes__(self):
        hops_hdr = (len(self._common_route) + 1).to_bytes(1, byteorder='little')
        common_route = b"".join(bytes(r) for r in self._common_route)
        payloads = b"".join(bytes(p) for p in self._payloads)
        return hops_hdr + common_route + payloads

    def set_common_route(self, value: HopList):
        self._common_route = value

    def iter_payloads(self):
        for payload in self._payloads:
            complete_route = [*self._common_route, payload._hop]
            yield payload._type, complete_route, payload._payload

    @staticmethod
    def decrypt(route : HopList, encrypted_payload : bytes, key : bytes):
        original_hop = route[-1]
        original_addr = original_hop.address
        decrypted = b""

        if original_hop.interface == MessageInterface.COMMS_INTERFACE_BLUETOOTH:
            # Attempt to decode
            addr_bytes = int.to_bytes(original_addr, 6, 'little')
            init_vector = addr_bytes[:3] + 13 * b"\x00"
            decrypted_payload = b""
            decrypted_address = None

            if len(encrypted_payload) % 16 != 0:
                raise DecryptionError

            for _ in range(len(encrypted_payload) // 16):
                cipher = AES.new(key, AES.MODE_CBC, init_vector)
                decrypted = cipher.decrypt(encrypted_payload)
                decrypted_oui = decrypted[:3]

                # Validate decrypted payload [CSIRO, MINEW]
                valid_ouis = [b"\x90\x97\xd8", b"\x3F\x23\xAC"]
                if not decrypted_oui in valid_ouis:
                    raise DecryptionError

                decrypted_payload += decrypted[3:]

                if decrypted_address is None:
                    # Set the  decrypted address on the first iteration
                    decrypted_address = addr_bytes[:3] + decrypted[:3]
                else:
                    # All other addresses should match the first address
                    if addr_bytes[:3] + decrypted[:3] != decrypted_address:
                        raise DecryptionError
            # Update the routing information
            address_fixed = int.from_bytes(decrypted_address, 'little')
            route[-1].update_address(address_fixed)
        else:
            raise NotImplementedError

        return route, decrypted_payload

    @classmethod
    def from_bytes(cls, packet_type: PayloadType, address: int, byte_buffer : bytes):
        if packet_type == PayloadType.PAYLOAD_INCOMING:
            # Payloads are prepended by route information
            num_hops = int(byte_buffer[0])
            byte_buffer = byte_buffer[1:]

            common_route = []
            # Extract common hops
            for n in range(num_hops - 1):
                hop_bytes = byte_buffer[n*Hop.INCOMING_SIZE: (n + 1)* Hop.INCOMING_SIZE]
                common_route.append(Hop.from_bytes(hop_bytes))
            byte_buffer = byte_buffer[(num_hops - 1)*Hop.INCOMING_SIZE:]
            # Extract individual payloads
            payloads = []
            while len(byte_buffer) > 0:
                payload_size = int(byte_buffer[0])
                payload_type = PayloadType(int(byte_buffer[1]))
                hop = Hop.from_bytes(byte_buffer[2:2+Hop.INCOMING_SIZE])
                payloads.append(cls.Payload(payload_type, hop, byte_buffer[2+Hop.INCOMING_SIZE:payload_size]))
                byte_buffer = byte_buffer[payload_size:]
            packet = cls(common_route, payloads)
        elif packet_type == PayloadType.PAYLOAD_OUTGOING:
            raise NotImplementedError
        else:
            # The bytes directly represent one of the payload types
            # There is no routing info, so construct dummy info based on the fact this packet must have come from serial node
            hop = Hop(address, MessageInterface.COMMS_INTERFACE_SERIAL, "default")
            hop.apply_metadata(0, 0, 30)
            payload = cls.Payload(packet_type, hop, byte_buffer)
            packet = cls([], [payload])
        return packet

if __name__ == '__main__':

    hop1 = Hop(10, MessageInterface.COMMS_INTERFACE_BLUETOOTH, "default")
    hop2 = Hop(10, MessageInterface.COMMS_INTERFACE_BLUETOOTH, "default")
    hop3 = Hop(10, MessageInterface.COMMS_INTERFACE_BLUETOOTH, "default")

    payload1 = PacpPacket.Payload(PayloadType.PAYLOAD_TDF3, hop1, b"\x00\x11\x22")
    payload2 = PacpPacket.Payload(PayloadType.PAYLOAD_TDF3, hop2, b"\x00\x11\x33")
    packet = PacpPacket([hop3], [payload1, payload2])

    int(b"\x33")

    for t, r, p in packet.iter_payloads():
        print(t, r, p)


