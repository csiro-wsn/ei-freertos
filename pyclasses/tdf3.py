#!/usr/bin/env python
''' TDF helper function for encoding/decoding in JSON format
and parsing TDF code
'''
__author__ = 'Ben Mackey'

import os
import sys
import urllib.request, urllib.error, urllib.parse
import json
import struct
import datetime
import FosTime
import socket
import ssl
# For conversion equations
import math
import operator

class tcolour:
    BLACK = '\033[90m'
    RED = '\033[91m'
    GREEN = '\033[92m'
    YELLOW = '\033[93m'
    BLUE = '\033[94m'
    PURPLE = '\033[95m'
    CYAN = '\033[96m'
    GREY = '\033[97m'
    END = '\033[0m'

    def disable(self):
        self.BLACK = ''
        self.RED = ''
        self.GREEN = ''
        self.YELLOW = ''
        self.BLUE = ''
        self.PURPLE = ''
        self.CYAN = ''
        self.GREY = ''
        self.END = ''


struct_c99 = {
    #'char':   'c',
    'double': 'd',
    'float':  'f',
    'int8':   'b',
    'int16':  'h',
    'int32':  'l',
    'int64':  'q',
    'uint8':  'B',
    'uint16': 'H',
    'uint32': 'L',
    'uint64': 'Q',
    'pascal': 'p',
    }

endian = '<'

lambdastr = 'lambda x: '
lambdastrf = 'lambda x: "%s" %% x'

default_server = 'https://tdf.csiro.au/tdf'
default_files = ['/var/tmp/tdf.json', '/tmp/tdf.json'] if os.name != 'nt' else [os.path.join(os.environ['TEMP'], 'tdf.json')]

class TdfParseException(Exception):
    '''Exception class for the TDF parsing library'''
    pass

class TdfBufferLengthError(TdfParseException):
    '''Exception when the buffer underruns whilst reading TDF'''
    def __init__(self, value, name, offset, timestamp=None):
        self.value = value # TDF ID number
        self.name = name # Sensor name
        self.buf_offset = offset # Buffer offset
        self.timestamp = timestamp
    def __str__(self):
        return 'Buffer underflow when reading TDF type %d (%s)' % ( self.value, self.name )

class TdfLookupError(TdfParseException):
    def __init__(self, value, offset):
        self.value = value
        self.buf_offset = offset
    def __str__(self):
        return 'TDF Type %d not found in TDF dictionary' % self.value

class TdfTimestampNotDefined(TdfParseException):
    def __init__(self, offset):
        self.buf_offset = offset
    def __str__(self):
        return 'Timestamp Offset used with no previous Timestamp defined'

class TdfNotLoaded(TdfParseException):
    def __init__(self, ):
        return
    def __str__(self):
        return 'TDF dictionary could not be found'

class Tdf:

    # 16 bit TDF values reserve the first 4 bits for flags
    # The two MSB define timestamp options
    # The remaining two bytes define array options
    FLAG_LOCALTIME  = 0x00
    FLAG_SEC_OFFSET = 0x01 << 2
    FLAG_MS_OFFSET  = 0x02 << 2
    FLAG_TIMESTAMP  = 0x03 << 2
    FLAG_TIME_MASK  = 0x03 << 2
    FLAG_TEMPORAL_ARRAY = 0x02
    FLAG_SPATIAL_ARRAY = 0x01
    FLAG_BITS = 4
    FLAG_MASK = 0xF000

    def __init__(self, tdfDict={}):
        self.tdfDict = tdfDict

    def loadTdf(self, url=default_server, useLocal=True, timeout=2, dbnames = False):
        extra = ''
        if dbnames:
            extra = '?dbnames=1'
        if url is None: url = default_server
        
        try:
            ssl._create_default_https_context = ssl._create_unverified_context
            jsonServer = urllib.request.urlopen(url+extra, timeout=timeout)
            self.tdfDict = json.load(jsonServer)
            print("TDF definitions loaded from", url)
            if useLocal: self.saveLocalTdf()
        except urllib.error.HTTPError as e:
            print("HTTP Error returned by server when loading " + (url+extra) + ":", e.code, file=sys.stderr)
            if useLocal: self.loadLocalTdf()
        except (urllib.error.URLError, socket.timeout) as e:
            print("Error connecting to server", url, file=sys.stderr)
            if useLocal: self.loadLocalTdf()
        except ValueError as e:
            print("Error reading dictionary from", url+extra, file=sys.stderr)
            if useLocal: self.loadLocalTdf()
        except Exception as e:
            print("Exception:", e, "(",type(e).__name__,")")
        finally:
            return not not self.tdfDict

    def saveLocalTdf(self, files=[]):
        files += default_files
        # Save JSON document
        for f in files:
            try:
                with open(f, 'w') as jsonDoc:
                    json.dump(self.tdfDict, jsonDoc)
                    break
            except IOError as e:
                    print("Error writing TDF data to file", f, file=sys.stderr)

    def loadLocalTdf(self, files=None):
        if files is None:
            files = []
        files += default_files
        for f in files:
            try:
                with open(f, 'r') as data:
                    self.tdfDict = json.load(data)
                    print("Loaded TDF data from file", f, file=sys.stderr)
                    return
            except IOError as e:
                continue
        print("Could not find local TDF definitions", file=sys.stderr)
        raise TdfNotLoaded()

    def sensorByName(self, name):
        """Returns the sensor that matches provided name"""
        name_keyed = {k['sensor']: k for k in self.tdfDict.values()}
        return name_keyed[name]

    def applyConversion(self, value, lambda_func = None, format_str = None):
        # Maybe move this function outside Tdf class?
        default_lambda = lambda x: x
        default_format = lambda x: "%d" % x
        # Conversion Equation
        if type(lambda_func) == type('str') or type(lambda_func) == type('str'):
            # Remove explicit typing
            if (lambda_func.startswith("cast:")):
                lambda_func = lambda_func.partition(')')[2]
            lambda_func = eval(lambdastr+lambda_func)
        elif type(lambda_func) == type(default_lambda):
            pass
        else:
            lambda_func = default_lambda
        try:
            converted_value = lambda_func(value)
        except:
            print("Error executing conversion equation:", sys.exc_info(), file=sys.stderr)
            converted_value = value
        # String Format
        if type(value) == type('str'):
            format_func = lambda x: str(x)
        elif type(format_str) == type('str') or type(format_str) == type('str'):
            format_func = eval(lambdastrf % format_str)
        elif type(format_str) == type(default_format):
            format_func = format_str
        else:
            format_func = default_format
        try:
            if isinstance(converted_value, bytes):
                formatted_value = format_func(int(converted_value))
            else:
                formatted_value = format_func(converted_value)
        except:
            print(converted_value)
            print("Error executing format string function: %s on {:}" % (format_str), sys.exc_info(), file=sys.stderr)
            formatted_value = str(value)
        return (converted_value, formatted_value)

    def parseTdf8(self, buffer, time=datetime.datetime.utcnow(), debug=False):
        ''' Parses TDF2 (8 bit) messages
        Takes TDF buffer and time
        Emits dict containing {'sensor', 'phenomenon', 'time', 'raw', 'converted', 'formatted'} '''
        if debug == True:
            debug = sys.stdout
        initialBufLen = len(buffer)
        if debug:
            c = tcolour()
        while True:
            # Is there still data in the buffer?
            if len(buffer) == 0:
                return

            # Store the current buffer length (for reporting in errors)
            currentBufLen = len(buffer)

            # Get the SensorID for the next reading
            sid = buffer[0]
            if debug: print(c.BLUE + ' '.join(['%02x' % b for b in buffer[:1]]) + c.END, end=' ', file=debug)
            buffer = buffer[1:]

            # Look up the SID in the tdf dictionary
            try:
                sensorInfo = self.tdfDict[str(sid)]
            except KeyError:
                if sid > 7:
                    if debug: print(c.RED + ' '.join(['%02x' % b for b in buffer]) + c.END, file=debug)
                    raise TdfLookupError(sid, initialBufLen-currentBufLen)
                return

            # Get the data from the buffer
            for p in sensorInfo['phenomena']:
                # Return if not enough bytes left
                if p['bytes'] > len(buffer):
                    if debug: print(c.RED + ' '.join(['%02x' % b for b in buffer]) + c.END, file=debug)
                    raise TdfBufferLengthError(sid, sensorInfo['sensor'], initialBufLen-currentBufLen)
                data = buffer[:p['bytes']]
                if debug: print(' '.join(['%02x' % b for b in data]), end=' ', file=debug)
                buffer = buffer[p['bytes']:]
                if p['ctype'] in struct_c99:
                    # Known c99 type
                    value = struct.unpack(endian+struct_c99[p['ctype']], data)[0]
                else:
                    # Not a ctype - possibly string or byte array
                    value = struct.unpack(endian+str(p['bytes'])+'s', data)[0]
                (converted_value, formatted_value) = self.applyConversion(value, p['conversionEq'], p['formatStr'])
                # Add value into array
                yield { 'sensor': sensorInfo['sensor'],
                        'phenomenon': p['phenomenon'],
                        'time': time,
                        'raw': value,
                        'converted': converted_value,
                        'formatted': formatted_value,
                        'tdf_id': sid }
        if debug: print('', file=debug)
        return

    def parseTdf16(self, buffer, time=datetime.datetime.utcnow(), timestamp=None, debug=False, combine=False):
        ''' Parses TDF3 (16 bit) messages
        Takes TDF buffer and time
        Emits dict containing {'sensor', 'phenomenon', 'time', 'raw', 'converted', 'formatted'} '''
        if debug == True:
            debug = sys.stdout
        if type(time) != datetime.datetime:
            raise TypeError("time must be of type DateTime")
        initialBufLen = len(buffer)
        if debug:
            c = tcolour()

        # Prints entire received packet in purple for debugging purposes.
        if debug:
            print("\r\n" + c.PURPLE + ' '.join(['%02x' % b for b in buffer]) + c.END, file=debug)
        
        while True:
            # Is there still data in the buffer?
            if len(buffer) == 0:
                return

            # Some useful variables
            arraySize = [ 1, 1 ] # temporal, spatial
            temporalOffset = datetime.timedelta(0)
            timestampOffset = datetime.timedelta(0)
            useTimestamp = False
            currentBufLen = len(buffer)

            # Get the SensorID for the next reading
            try:
                (sid,) = struct.unpack(endian+struct_c99['uint16'], buffer[:2])
                if debug: print(c.BLUE + ' '.join(['%02x' % b for b in buffer[:2]]) + c.END, end=' ', file=debug)
                buffer = buffer[2:]
            except struct.error:
                if len(buffer) == 1 and ord(buffer) == 0:
                    # Extra zero byte in buffer - ignore this.
                    return
                if debug: print(c.RED + ' '.join(['%02x' % b for b in buffer]) + c.END, file=debug)
                raise TdfBufferLengthError(0, 'None', initialBufLen-currentBufLen, timestamp)
            except:
                if debug: print(c.RED + ' '.join(['%02x' % b for b in buffer]) + c.END, file=debug)
                raise TdfBufferLengthError(0, 'None', initialBufLen-currentBufLen, timestamp)

            # Mask out flag bits
            flags = ( sid & Tdf.FLAG_MASK ) >> (16-Tdf.FLAG_BITS)
            sid = sid & ~Tdf.FLAG_MASK
            #print "flags: %x sid: %x" % (flags, sid)

            # Look up the SID in the tdf dictionary
            try:
                sensorInfo = self.tdfDict[str(sid)]
            except KeyError:
                if sid == 0 or sid == 4095: # 0xffff - 0xf000 = 4095
                    continue
                if debug: print(c.RED + ' '.join(['%02x' % b for b in buffer]) + c.END, end=' ', file=debug)
                raise TdfLookupError(sid, initialBufLen-currentBufLen)

            # Check for special flags in the SensorID
            if ( flags & Tdf.FLAG_TIME_MASK ) > 0:
                if ( flags & Tdf.FLAG_TIME_MASK ) == Tdf.FLAG_TIMESTAMP:
                    # Timestamp for reading
                    # [ sid | 4 byte fos_time | 2 byte ms | data ]
                    try:
                        (fos_time, sf) = struct.unpack('<LH', buffer[:6])
                    except:
                        if debug: print(c.RED + ' '.join(['%02x' % b for b in buffer]) + c.END, file=debug)
                        raise TdfBufferLengthError(sid, sensorInfo['sensor'], initialBufLen-currentBufLen, timestamp)
                    timestamp = FosTime.fos2utc(fos_time) + datetime.timedelta(milliseconds=((sf/65536.0)*1000.0))
                    timestamp = timestamp.replace(tzinfo=None)
                    if debug: print(c.GREEN + ' '.join(['%02x' % b for b in buffer[:6]]) + c.END, end=' ', file=debug)
                    buffer = buffer[6:]
                    useTimestamp = True
                elif ( flags & Tdf.FLAG_TIME_MASK ) == Tdf.FLAG_MS_OFFSET:
                    # Offset from last timestamp in us
                    # [ sid | 2 byte ms | data ]
                    try:
                        (sf,) = struct.unpack('<H', buffer[:2])
                    except:
                        if debug: print(c.RED + ' '.join(['%02x' % b for b in buffer]) + c.END, file=debug)
                        raise TdfBufferLengthError(sid, sensorInfo['sensor'], initialBufLen-currentBufLen, timestamp)
                    if timestamp is not None:
                        #An explicit time stamp is provided. The offset is treated as a future offset relative to 'timestamp'
                        timestampOffset = datetime.timedelta(milliseconds=((sf/65536.0)*1000.0))
                        if debug: print(c.GREEN + ' '.join(['%02x' % b for b in buffer[:2]]) + c.END, end=' ', file=debug)
                        buffer = buffer[2:]
                        useTimestamp = True
                    else:
                        #No explicit timestamp. The offset is treated as defining a delay before 'time', or negative/past offset
                        timestampOffset = datetime.timedelta(milliseconds=((-sf/65536.0)*1000.0))
                        if debug: print(c.GREEN + ' '.join(['%02x' % b for b in buffer[:2]]) + c.END, end=' ', file=debug)
                        buffer = buffer[2:]
                        useTimestamp = False
                        
                elif ( flags & Tdf.FLAG_TIME_MASK ) == Tdf.FLAG_SEC_OFFSET:
                    # Offset from last timestamp in s
                    # [ sid | 2 byte sec | data ]
                    try:
                        (sec,) = struct.unpack('<H', buffer[:2])
                    except:
                        if debug: print(c.RED + ' '.join(['%02x' % b for b in buffer]) + c.END, file=debug)
                        raise TdfBufferLengthError(sid, sensorInfo['sensor'], initialBufLen-currentBufLen, timestamp)
                    if timestamp is not None:
                        #An explicit time stamp is provided. The offset is treated as a future offset relative to 'timestamp'
                        timestampOffset = datetime.timedelta(seconds=sec)
                        if debug: print(c.GREEN + ' '.join(['%02x' % b for b in buffer[:2]]) + c.END, end=' ', file=debug)
                        buffer = buffer[2:]
                        useTimestamp = True
                    else:                        
                        #No explicit timestamp. The offset is treated as defining a delay before 'time', or negative/past offset
                        timestampOffset = datetime.timedelta(seconds=sec*-1)                        
                        if debug: print(c.GREEN + ' '.join(['%02x' % b for b in buffer[:2]]) + c.END, end=' ', file=debug)
                        buffer = buffer[2:]
                        useTimestamp = False
                        
            if flags & Tdf.FLAG_TEMPORAL_ARRAY:
                # Structure of temporal array is:
                # [ sid | reading offset | size | data ]
                try:
                    (temporalOffset, arraySize[0]) = struct.unpack('<HB', buffer[:3])
                except:
                    if debug: print(c.RED + ' '.join(['%02x' % b for b in buffer]) + c.END, file=debug)
                    raise TdfBufferLengthError(sid, sensorInfo['sensor'], initialBufLen-currentBufLen, timestamp)
                temporalOffset = datetime.timedelta(milliseconds=((temporalOffset/65536.0)*1000.0))
                if debug: print(c.CYAN + ' '.join(['%02x' % b for b in buffer[:3]]) + c.END, end=' ', file=debug)
                buffer = buffer[3:]
            if flags & Tdf.FLAG_SPATIAL_ARRAY:
                # Structure of spatial array is:
                # [ sid | size | data ]
                # The data must be a TDF Jumbo - format TBD
                try:
                    arraySize[1] = ord(buffer[0])
                    if debug: print(c.CYAN + ' '.join(['%02x' % b for b in buffer[:1]]) + c.END, end=' ', file=debug)
                    buffer = buffer[1:]
                except:
                    if debug: print(c.RED + ' '.join(['%02x' % b for b in buffer]) + c.END, file=debug)
                    raise TdfBufferLengthError(sid, sensorInfo['sensor'], initialBufLen-currentBufLen, timestamp)

            # Before processing, check there are enough bytes in the arrays
            if 'bytes' in sensorInfo and sensorInfo['bytes'] is not None \
              and len(buffer) < arraySize[0]*arraySize[1]*sensorInfo['bytes']:
                if debug: print(c.RED + ' '.join(['%02x' % b for b in buffer]) + c.END, file=debug)
                raise TdfBufferLengthError(sid, sensorInfo['sensor'], initialBufLen-currentBufLen, timestamp)

            # Get the data from the buffer
            for temporal in range(arraySize[0]):
                for spatial in range(arraySize[1]):
                    phenomenon = {}
                    for p in sensorInfo['phenomena']:
                        if p['ctype'] == 'string':
                            # Read until null character found
                            p['bytes'] = len(buffer[:buffer.find(b'\x00')]) + 1
                        if p['ctype'] == 'pascal':
                            # First byte is length
                            p['bytes'] = buffer[0].to_bytes(1, byteorder="big")[0]+1
                        # Return if not enough bytes left
                        if p['bytes'] > len(buffer):
                            # Should never get here due to check before for loop
                            if debug: print(c.RED + ' '.join(['%02x' % b for b in buffer]) + c.END, file=debug)
                            raise TdfBufferLengthError(sid, sensorInfo['sensor'], initialBufLen-currentBufLen, timestamp)
                        data = buffer[:p['bytes']]
                        if debug: print(' '.join(['%02x' % b for b in data]), end=' ', file=debug)
                        buffer = buffer[p['bytes']:]
                        if p['ctype'] in struct_c99:
                            # Known c99 type
                            if p['ctype'] == 'pascal':
                                #(value,) = struct.unpack(endian+str(p['bytes'])+struct_c99[p['ctype']], data)
                                value = data[1:] # Ignore length byte
                            else:
                                (value,) = struct.unpack(endian+struct_c99[p['ctype']], data)
                        elif p['ctype'] == 'string':
                            value = data[:-1] # Ignore null
                        else:
                            # Not a ctype - possibly string or byte array
                            (value,) = struct.unpack(endian+str(p['bytes'])+'s', data)
                        (converted_value, formatted_value) = self.applyConversion(value, p.get('conversionEq'), p.get('formatStr'))

                        if (combine):
                            # combine to jumbo
                            phenomenon[p['phenomenon']] = { 'raw': value, 'converted': converted_value, 'formatted': formatted_value}
                        else:
                            # Add value into array
                            yield { 'sensor': sensorInfo['sensor'],
                               'phenomenon': p['phenomenon'],
                               'time': timestamp + timestampOffset + temporal*temporalOffset if useTimestamp else time +timestampOffset + temporal*temporalOffset,
                               'raw': value,
                               'converted': converted_value,
                               'formatted': formatted_value,
                               'tdf_id': sid,
                               'flags': flags }

                    if debug:
                        print('', file=debug, flush=True)

                    if (combine):
                        yield {'sensor': sensorInfo['sensor'],
                               'tdf_id': sid,
                               'time': timestamp + timestampOffset + temporal*temporalOffset if useTimestamp else time +timestampOffset + temporal*temporalOffset,
                               'phenomena': phenomenon,
                                }
        if debug: print('', file=debug, flush=True)
        return
