#!/usr/bin/env python3

import configparser
import argparse

from ArgparseMac import MacAddressType

class InvalidConfiguration(Exception):
    '''Invalid configuration file supplied'''
    def __init__(self, message):
        self._message = message

    def __str__(self):
        return self._message

class NodeFilter:
    '''
        Filters devices by their MAC address and MAJOR/MINOR
        MAJOR/MINOR is determined via parsing TDF's
    '''
    PASSED = 0
    FAILED = 1
    UNKNOWN = 2
    
    def __init__(self, filter_config):
        if filter_config != None:
            config = configparser.ConfigParser(allow_no_value=True)
            config.read(filter_config)
            # Construct filter categories
            try:
                self.encryption_keys = [bytes.fromhex(x) for x in config['ENCRYPTION_KEYS']]
                self.whitelisted_addresses = self._range_parser(config['ADDRESS_WHITELIST'])
                self.blacklisted_addresses = self._range_parser(config['ADDRESS_BLACKLIST'])
                self.whitelisted_majors = self._range_parser(config['MAJOR_WHITELIST'])
                self.blacklisted_majors = self._range_parser(config['MAJOR_BLACKLIST'])
            except KeyError:
                raise InvalidConfiguration("Missing configuration file heading")
            # Observed Devices
            self._majors = {}
        else:
            self.encryption_keys = []
            self.whitelisted_addresses = []
            self.blacklisted_addresses = []
            self.whitelisted_majors = []
            self.blacklisted_majors = []   
            self._majors = {}

    def _range_parser(self, config_section):
        mac_parser = MacAddressType()
        r = []
        for whitelist in config_section:
            if '-' in whitelist:
                # Key is a range
                (range_start, range_end) = whitelist.split('-')
                try:
                    r_start = mac_parser(range_start)
                    r_end = mac_parser(range_end)
                except argparse.ArgumentTypeError:
                    raise InvalidConfiguration("Invalid MAC Address {:s} {:s}".format(range_start, range_end))
                if r_end < r_start:
                    raise InvalidConfiguration("Range is invalid {:s} < {:s}".format(range_end, range_start))
                r.append((r_start, r_end))
            else:
                # Key is a single value
                try:
                    address = mac_parser(whitelist)
                except argparse.ArgumentTypeError:
                    raise InvalidConfiguration("Invalid MAC Address {:s}".format(whitelist))
                r.append((address, address))
        return r

    def _values_in_ranges(self, value, ranges):
        for r in ranges:
            if r[0] <= value and value <= r[1]:
                return True
        return False

    def consume_tdf(self, address, tdf_point):
        '''
            Consume information from TDF's that can be used as a basis for filtering
        '''
        if 'major' in tdf_point['phenomena']:
            self._majors[address] = tdf_point['phenomena']['major']['converted']

    def filter(self, address):
        '''
            Validate a device against the provided configuration file
            NodeFilter.UNKNOWN is returned when MAJOR/MINOR filtering is enabled, but a MAJOR/MINOR has not yet been observed for that device
        '''
        # Check against address whitelist
        if len(self.whitelisted_addresses) > 0 and not self._values_in_ranges(address, self.whitelisted_addresses):
            return self.FAILED
        # Check against address blacklist
        if len(self.blacklisted_addresses) > 0 and self._values_in_ranges(address, self.blacklisted_addresses):
            return self.FAILED
        # Quick check against no major filtering
        if len(self.whitelisted_majors) == 0 and len(self.blacklisted_majors) == 0:
            return self.PASSED
        # Check if we haven't yet determined the major 
        if address not in self._majors:
            return self.UNKNOWN
        # Check against major whitelists
        if len(self.whitelisted_majors) > 0 and not self._values_in_ranges(self._majors[address], self.whitelisted_majors):
            return self.FAILED
        # Check against major blacklists
        if len(self.blacklisted_majors) > 0 and self._values_in_ranges(self._majors[address], self.blacklisted_majors):
            return self.FAILED
        # All checks have passed
        return self.PASSED

    