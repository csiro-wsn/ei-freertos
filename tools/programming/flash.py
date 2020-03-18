#!/usr/bin/env python3

import argparse
import sys
import binascii
import os
import csv

import programming.jlink


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


if __name__ == '__main__':
    Flasher()
