#!/usr/bin/env python3

__author__ = "Jordan Yates"

import argparse
import re

class MacAddressType(object):
    mac_colon = re.compile(r"^(([0-9,a-f,A-F]{2}:){5})[0-9,a-f,A-F]{2}$") # Groupings of 2 HEX characters with colon seperators
    mac_hex = re.compile(r"^[0-9,a-f,A-F]{12}$") # Exactly 12 HEX characters
    mac_int = re.compile(r"^[0-9]{1,15}$") # Between 1 and 15 integers

    def __init__(self):
        pass

    def __call__(self, value):
        if self.mac_colon.match(value):
            return int("".join(value.split(":")), base=16)
        elif self.mac_hex.match(value):
            return int(value, base=16)
        elif self.mac_int.match(value):
            return int(value)
        else:
            raise argparse.ArgumentTypeError("{} is not a valid MAC address".format(value))
