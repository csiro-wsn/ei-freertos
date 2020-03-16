#!/usr/bin/env python3

import argparse
import sys
import binascii
import os
import csv

import programming.jlink
from programming.constants import get_struct_fields, construct_constant_blob

CONSTANTS_KEY = "0x1337BEEF"


def constants_extract(csv_file, mac):
    c = None
    with open(csv_file) as inf:
        reader = csv.reader(inf.readlines())
        header = reader.__next__()

    with open(csv_file, 'w') as outf:
        writer = csv.writer(outf)
        writer.writerow(header)
        for line in reader:
            if int(line[0], 0) == mac:
                c = {header[n]: v for n, v in enumerate(line)}
                writer.writerow(line)
                break
            if int(line[0], 0) == 0:
                c = {header[n]: v for n, v in enumerate(line)}
                line[0] = "0x{:012x}".format(mac)
                writer.writerow(line)
                break
            writer.writerow(line)
        writer.writerows(reader)
    if c == None:
        print("No constants remaining in file")
        sys.exit(-1)
    return c


def get_fields_values(constants_csv, mac, header):
    fields = get_struct_fields(header, 'xDeviceConstants_t')
    # Check for the base case (no external constants to load)
    if (len(fields) == 2) and (fields[0]['name'] == 'ulKey') and (fields[1]['name'] == 'ucApplicationImageSlots'):
        values = {'ulKey': CONSTANTS_KEY, 'ucApplicationImageSlots': "2"}
    # Otherwise we need to load in constants from the file
    else:
        # Get the constants associated with a MAC address
        values = constants_extract(constants_csv, mac)
        values['ulKey'] = CONSTANTS_KEY
    return (fields, values)


class Flasher(object):

    def __init__(self):
        parser = argparse.ArgumentParser(description='Flash data to devices')
        parser.add_argument('command', help='Subcommand to run')
        # parse_args defaults to [1:] for args, but you need to
        # exclude the rest of the args too, or validation will fail
        args = parser.parse_args(sys.argv[1:2])
        if not hasattr(self, args.command):
            print("Unrecognized command")
            parser.print_help()
            exit(1)
        # use dispatch pattern to invoke method with same name
        getattr(self, args.command)()

    def app(self):
        parser = argparse.ArgumentParser(description='Flash application binaries')
        parser.add_argument('--target', dest='target', type=str, required=True, help='Hardware Target')
        parser.add_argument('--binary', dest='binary', type=str, required=True, help='Binary file location')
        parser.add_argument('--jlink', dest='jlink', type=int, help='JLink serial number')
        parser.add_argument('--remote', dest='remote', type=str, help='JLink remote address')
        # Now that we're inside a subcommand, ignore the first TWO arguments (flash.py, app)
        args = parser.parse_args(sys.argv[2:])

        # Get the appropriate jlink interface for this target
        jlink = programming.jlink.target_interface(args.target)()
        # Open a connection, flash the binary, disconnect
        jlink.connect(args.jlink, args.remote)
        jlink.flash_prog(args.binary)
        jlink.disconnect()

    def bootloader(self):
        parser = argparse.ArgumentParser(description='Flash bootloader image')
        parser.add_argument('--target', dest='target', type=str, required=True, help='Hardware Target')
        parser.add_argument('--binary', dest='binary', type=str, required=True, help='Bootloader file location')
        parser.add_argument('--jlink', dest='jlink', type=int, help='JLink serial number')
        parser.add_argument('--remote', dest='remote', type=str, help='JLink remote address')
        args = parser.parse_args(sys.argv[2:])

        # Get the appropriate jlink interface for this target
        jlink = programming.jlink.target_interface(args.target)()
        # Open a connection, flash the bootloader, disconnect
        jlink.connect(args.jlink, args.remote)
        jlink.flash_bootloader(args.binary)
        jlink.disconnect()

    def constants(self):
        parser = argparse.ArgumentParser(description='Flash bootloader image')
        parser.add_argument('--target', dest='target', type=str, required=True, help='Hardware Target')
        parser.add_argument('--header', dest='header', type=str, required=True, help='Device constants header file')
        parser.add_argument('--folder', dest='folder', type=str, required=True, help='Folder containing device constants')
        parser.add_argument('--jlink', dest='jlink', type=int, help='JLink serial number')
        parser.add_argument('--remote', dest='remote', type=str, help='JLink remote address')
        args = parser.parse_args(sys.argv[2:])

        # Get the appropriate jlink interface for this target
        jlink = programming.jlink.target_interface(args.target)()

        # Open a connection, read the MAC address
        jlink.connect(args.jlink, args.remote)
        mac = jlink.mac_address()

        f_constants = os.path.join(args.folder, "{:s}.csv".format(args.target))
        (fields, values) = get_fields_values(f_constants, mac, args.header)
        binary_blob = construct_constant_blob(fields, values)

        # Flash the constants, disconnect
        print("MAC Address {:012X}: {:}".format(mac, binascii.hexlify(binary_blob)))
        jlink.flash_constants(binary_blob)
        jlink.disconnect()


if __name__ == '__main__':
    Flasher()
