#!/usr/bin/env python3

"""
jink.py

Interface for interacting with PACP devices through python rather than JLinkExe.
Utilises the JLink Library through the pylink package

"""
__author__ = "Jordan Yates"

import argparse
import os
import pylink


flash_progress = b"Compare"


def flash_print_progress(action, progress_string, percentage):
    global flash_progress
    if flash_progress == b"Compare":
        if (action == b"Compare") and (progress_string is not None):
            # Output the progress string on the current line
            print("\tCompare: {:s}".format(progress_string.decode('utf-8')), end='\r', flush=True)
        elif action != b"Compare":
            flash_progress = action
            print("\tCompare: 100%{:s}".format(60*" "))

    if flash_progress == b"Erase":
        if (action == b"Erase") and (progress_string is not None):
            # Output the progress string on the current line
            print("\t  Erase: {:3d}% {:s}".format(percentage, progress_string.decode('utf-8')), end='\r', flush=True)
        if action == b"Program":
            flash_progress = action
            print("\t  Erase: 100%{:s}".format(60*" "))

    if flash_progress == b"Program":
        if (action == b"Program") and (progress_string is not None):
            print("\tProgram: {:s}".format(progress_string.decode('utf-8')), end='\r', flush=True)
        if action == b"Verify":
            flash_progress = action
            print("\tProgram: 100%{:s}".format(60*" "))

    if flash_progress == b"Verify":
        if (action == b"Verify") and (progress_string is not None) and (percentage != 100):
            # Output the progress string on the current line
            print("\t Verify: {:s} {:d}%".format(progress_string.decode('utf-8'), percentage), end='\r', flush=True)
        if (action == b"Verify") and (percentage == 100):
            print("\t Verify: 100%{:s}".format(60*" "))
            flash_progress = None


class JLinkInterface(object):

    pylink_interface = pylink.JLinkInterfaces.SWD
    pylink_name = ""

    jlink_interface = "SWD"
    jlink_speed = 4000
    jlink_name = ""

    gdb_name = ""

    flash_address = 0x00
    constants_address = 0xFFFFFFFF
    bootloader_address = 0xFFFFFFFF

    def __init__(self):
        self.jlink = pylink.JLink()
        self.jlink_sn = 0

    def emulators(self):
        return self.jlink.connected_emulators()

    def connect(self, serial_no, remote_addr=None):
        self.validate_unique_jlink(serial_no)
        self.jlink_sn = serial_no
        self.jlink.open(serial_no=self.jlink_sn, ip_addr=remote_addr)
        self.jlink.set_tif(self.pylink_interface)
        self.jlink.connect(chip_name=self.pylink_name, speed=self.jlink_speed, verbose=True)

    def disconnect(self):
        # Disable the debug interface to return current consumption to normal.
        # https://developer.arm.com/docs/100572/latest/programmers-model/register-descriptions/debug-port-registers
        # I don't fully know how this works, but the idea is this:
        # According to this comment: https://devzone.nordicsemi.com/f/nordic-q-a/45441/nrf52-cannot-exit-from-debug-interface-mode
        # the debug interface can be turned off with `writeDP 1 0` with JLinkExe. This works, but there
        # doesn't exists a pylink command to do this. Register 1 is at address 0x04, and we still write
        # the data to 0. I just fiddled with the order of commands until everything worked.
        self.jlink.reset(halt=False)
        self.jlink.coresight_configure(perform_tif_init=False)
        self.jlink.coresight_read(reg=0x04, ap=True)
        self.jlink.coresight_write(reg=0x04, data=0x00000000, ap=True)
        self.jlink.close()

    def flash_prog(self, binary):
        self.jlink.flash_file(binary, self.flash_address, on_progress=flash_print_progress,  power_on=True)

    def flash_bootloader(self, bootloader):
        self.jlink.flash_file(bootloader, self.bootloader_address, on_progress=flash_print_progress, power_on=True)

    def flash_constants(self, constants):
        self.jlink.flash(constants, self.constants_address)

    def mac_address(self):
        raise NotImplementedError()

    def validate_unique_jlink(self, serial_no):
        num_emulators = len(self.emulators())
        if (serial_no == None) and (num_emulators > 1):
            raise Exception("Detected {:d} emulators but no serial number was specified".format(num_emulators))


class JLinkInterfaceEFR(JLinkInterface):
    constants_address = 0xFE00000  # USER DATA Page
    bootloader_address = 0x00  # .s37 file includes bootloader offset

    def mac_address(self):
        address_location = 0x0FE081D8
        a = self.jlink.memory_read(address_location, 6)
        return int.from_bytes(a, byteorder='little', signed=False)


class JLinkInterfaceEFR32MG12(JLinkInterfaceEFR):
    pylink_name = "EFR32MG12PxxxF1024"
    jlink_name = "EFR32MG12P332F1024GL125"
    gdb_name = "EFR32MG12P332F1024GL125"


class JLinkInterfaceEFR32BG13(JLinkInterfaceEFR):
    pylink_name = "EFR32BG13PxxxF512"
    jlink_name = "EFR32BG13P632F512GM48"
    gdb_name = "EFR32BG13P632F512GM48"


class JLinkInterfaceNRF(JLinkInterface):
    constants_address = 0x10001080  # UICR CUSTOMER[0]
    gdb_name = "nrf52"

    def flash_bootloader(self, bootloader):
        raise NotImplementedError()

    def mac_address(self):
        address_type_location = 0x100000A0
        address_location = 0x100000A4
        t = self.jlink.memory_read(address_type_location, 4)
        a = self.jlink.memory_read(address_location, 6)

        t = int.from_bytes(t, byteorder='little', signed=False)
        a = int.from_bytes(a, byteorder='little', signed=False)
        if (t & 0x01):
            return 0xC00000000000 | a
        else:
            return a


class JLinkInterfaceNRF52840(JLinkInterfaceNRF):
    pylink_name = "nRF52840_xxAA"
    jlink_name = "nrf52840_xxAA"


class JLinkInterfaceNRF52832(JLinkInterfaceNRF):
    pylink_name = "nRF52832_xxAA"
    jlink_name = "nrf52832_xxAA"


class JLinkInterfaceLinux(JLinkInterface):
    pylink_name = "none"
    jlink_name = "none"
    gdb_name = "none"

    def connect(self, serial_no):
        return

    def disconnect(self):
        return

    def flash_prog(self, binary):
        return

    def flash_constants(self, constants):
        return


def target_interface(target):
    target_mapping = {
        "ceresig": JLinkInterfaceEFR32BG13,
        "ceresat": JLinkInterfaceNRF52840,
        "ceresat_old": JLinkInterfaceNRF52840,
        "thunderboard2": JLinkInterfaceEFR32MG12,
        "nrf52840dk": JLinkInterfaceNRF52840,
        "bleacon": JLinkInterfaceNRF52840,
        "bleatag": JLinkInterfaceNRF52840,
        "minew": JLinkInterfaceNRF52832,
        "nrf52832dk": JLinkInterfaceNRF52832,
        "linux": JLinkInterfaceLinux,
        "argon": JLinkInterfaceNRF52840,
        "xenon": JLinkInterfaceNRF52840
    }
    return target_mapping[target]


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description='Target jlink information')
    parser.add_argument('--target', dest='target', type=str, required=True, help='Hardware Target')
    parser.add_argument('--print', '-p', dest='print', required=True, choices=['list', 'interface', 'speed', 'cpu', 'gdb_cpu'])
    args = parser.parse_args()

    jlink = target_interface(args.target)()

    if args.print == 'list':
        for e in jlink.emulators():
            print("\t{}: {}".format(e.SerialNumber, e.acProduct.decode('utf-8')))
    if args.print == 'interface':
        print(jlink.jlink_interface)
    if args.print == 'speed':
        print(jlink.jlink_speed)
    if args.print == 'cpu':
        print(jlink.jlink_name)
    if args.print == 'gdb_cpu':
        print(jlink.gdb_name)
