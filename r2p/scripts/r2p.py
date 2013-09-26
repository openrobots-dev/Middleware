# TODO: Check BootlaoderMsg checksums
# TODO: Add BootlaoderMsg sequence number check

import sys, os, io, time, struct, string, random
import subprocess
import serial
import socket
import logging
from helpers import *

from elftools.elf.elffile import ELFFile
from elftools.construct.macros import FlagsEnum
from elftools.elf.sections import SymbolTableSection

MODULE_NAME_MAX_LENGTH = 7
NODE_NAME_MAX_LENGTH = 8
TOPIC_NAME_MAX_LENGTH = 16
IHEX_MAX_DATA_LENGTH = 16

APP_THREAD_SYMBOL = 'app_main'
THREAD_PC_OFFSET = 1
APP_CONFIG_SYMBOL = 'app_config'

_MODULE_NAME = ''.join([ random.choice(string.ascii_letters + string.digits + '_')
                         for x in range(MODULE_NAME_MAX_LENGTH) ])

#==============================================================================

def _get_section_address(elffile, name):
    for section in elffile.iter_sections():
        if section.name == name:
            return section.header['sh_addr']
    raise RuntimeError('Section "%s" not found' % name)


def _get_function_address(elffile, name):
    dwarfinfo = elffile.get_dwarf_info()
    for CU in dwarfinfo.iter_CUs():
        for DIE in CU.iter_DIEs():
            try:
                if DIE.tag == 'DW_TAG_subprogram' and DIE.attributes['DW_AT_name'].value == name:
                    return int(DIE.attributes['DW_AT_low_pc'].value) + THREAD_PC_OFFSET
            except KeyError: continue
    raise RuntimeError('Symbol "%s" not found' % name)


def _get_symbol_address(elffile, name):
    for section in elffile.iter_sections():
        if not isinstance(section, SymbolTableSection):
            continue
        for symbol in section.iter_symbols():
            if symbol.name == name:
                return symbol['st_value']
    raise RuntimeError('Symbol "%s" not found' % name)


def _get_variable_address(elffile, name):
    dwarfinfo = elffile.get_dwarf_info()
    for CU in dwarfinfo.iter_CUs():
        for DIE in CU.iter_DIEs():
            try:
                if DIE.tag == 'DW_TAG_variable' and DIE.attributes['DW_AT_name'].value == name:
                    value = DIE.attributes['DW_AT_location'].value
                    # FIXME: Handmade address conversion (I don't know how to manage this with pyelftools...)
                    assert value[0] == 3
                    return (value[4] << 24) | (value[3] << 16) | (value[2] << 8) | value[1]
            except KeyError: continue
    raise RuntimeError('Symbol "%s" not found' % name)


def _get_variable_size(elffile, name):
    dwarfinfo = elffile.get_dwarf_info()
    offset = None
    for CU in dwarfinfo.iter_CUs():
        for DIE in CU.iter_DIEs():
            try:
                if DIE.tag == 'DW_TAG_variable' and DIE.attributes['DW_AT_name'].value == name:
                    offset = DIE.attributes['DW_AT_type'].value
                    break
            except KeyError: continue
        else: continue
        break
    else: raise RuntimeError('Symbol "%s" not found' % name)
    
    for DIE in CU.iter_DIEs():
        try:
            if DIE.tag == 'DW_TAG_const_type' and DIE.offset == offset:
                offset = DIE.attributes['DW_AT_type'].value
                break
        except KeyError: continue
    else: pass # no separate struct/class type definition
    
    for DIE in CU.iter_DIEs():
        try:
            if DIE.tag == 'DW_TAG_typedef' and DIE.offset == offset:
                offset = DIE.attributes['DW_AT_type'].value
                break
        except KeyError: continue
    else: pass # no typedef in C++
    
    for DIE in CU.iter_DIEs():
        try:
            if DIE.tag == 'DW_TAG_structure_type' and DIE.offset == offset:
                size = DIE.attributes['DW_AT_byte_size'].value
                break
        except KeyError: continue
    else: raise RuntimeError('Cannot find structure type of variable "%s"' % name)
    
    return size


if __name__ == '__main__':
    with open('/home/texzk/openrobots/R2P_IMU_test_mw/apps/pub_led/build/pub_led.elf', 'r') as f:
        elffile = ELFFile(f)
        addr = _get_variable_size(elffile, APP_CONFIG_SYMBOL)

#==============================================================================

class Checksummer:
    def __init__(self):
        self.__accumulator = 0
        
    def __int__(self):
        return self.compute_checksum()
        
    def compute_checksum(self):
        return (0x100 - (self.__accumulator)) & 0xFF
    
    def add_uint(self, value):
        value = int(value)
        while value > 0:
            self.__accumulator = (self.__accumulator + value) & 0xFF
            value >>= 8
    
    def add_int(self, value, size=4):
        assert size > 0
        value = int(value) &  (1 << (8 * size)) - 1
        self.add_uint(value)
    
    def add_bytes(self, chunk):
        for b in chunk:
            self.__accumulator = (self.__accumulator + ord(b)) & 0xFF
            
    def check(self, checksum):
        if checksum != self.compute_checksum():
            raise ValueError('Checksum is 0x%0.2X, expected 0x%0.2X' %
                             (checksum, self.compute_checksum()))
    
#==============================================================================

class Serializable:
    
    def __repr(self):
        return str(self.__dict__)
    
    def marshal(self):
        raise NotImplementedError()
        # return 'data'
    
    def unmarshal(self, data):
        raise NotImplementedError()
    
#==============================================================================

class Transport:
    
    class TypeEnum:
        MESSAGE         = 0
        ADVERTISEMENT   = 1
        SUBSCRIPTION    = 2
        STOP            = 3
        RESET           = 4
    
    def send_message(self, topic, payload):
        raise NotImplementedError()
    
    def send_advertisement(self, topic):
        raise NotImplementedError()
    
    def send_subscription(self, topic, queue_length):
        raise NotImplementedError()
    
    def send_stop(self):
        raise NotImplementedError()
    
    def send_reboot(self):
        raise NotImplementedError()
    
    def recv(self):
        raise NotImplementedError()
        # return (type, 'topic', 'payload')

#==============================================================================

class LineIO:
    
    def open(self):
        raise NotImplementedError()
        
    def close(self):
        raise NotImplementedError()
    
    def readln(self):
        raise NotImplementedError()
        # return line
    
    def writeln(self, line):
        raise NotImplementedError()
        
#==============================================================================

class SerialLineIO(LineIO):
    
    def __init__(self, dev_path, baud):
        self.__ser = None
        self.__sio = None
        self.__dev_path = dev_path
        self.__baud = baud
    
    def open(self):
        self.__ser = serial.Serial(self.__dev_path, self.__baud)
        self.__sio = io.TextIOWrapper(buffer = io.BufferedRWPair(self.__ser, self.__ser, 1),
                                      newline = '\r\n',
                                      line_buffering = True,
                                      encoding = 'ascii')
        
    def close(self):
        self.__ser.close()
        self.__ser = None
        self.__sio = None
    
    def readln(self):
        line = self.__sio.readline().rstrip('\r\n')
        logging.debug("%s >>> '%s'" % (self.__dev_path, line))
        return line
    
    def writeln(self, line):
        logging.debug("%s <<< '%s'" % (self.__dev_path, line))
        self.__sio.write(unicode(line + '\n'));
    
#==============================================================================
    
class StdLineIO(LineIO):
    
    def open(self):
        pass
        
    def close(self):
        pass
    
    def readln(self):
        line = raw_input()
        logging.debug("stdin <<< '%s'" % line)
        return line
    
    def writeln(self, line):
        logging.debug("stdout >>> '%s'" % line)
        print line

#==============================================================================

class TCPLineIO(LineIO):
    
    def __init__(self, address, port):
        self.__socket = None
        self.__fp = None
        self.__address = address
        self.__port = port
    
    def open(self):
        self.__socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        self.__socket.connect((self.__address, self.__port))
        self.__fp = self.__socket.makefile()
        
        time.sleep(1)
        
    def close(self):
        self.__socket.close()
        self.__fp = None
    
    def readln(self):
        line = self.__fp.readline().rstrip('\r\n')
        logging.debug("%s >>> '%s'" % (self.__address, line))
        return line
     
    def writeln(self, line):
        logging.debug("%s <<< '%s'" % (self.__address, line))
        self.__fp.write(unicode(line + '\n'))
        self.__fp.flush()
        
#==============================================================================

class DebugTransport(Transport):
    
    class MsgParser:
        def __init__(self, line):
            self._line = str(line)
            self._linelen = len(self._line)
            self._offset = 0
        
        def _check_length(self, length):
            assert length >= 0
            endx = self._offset + length
            if self._linelen < endx:
                raise ValueError("Expected %d chars at '%s'[%d:%d] == '%s' (%d chars less)" %
                                 (length, self._line, self._offset, endx,
                                  self._line[self._offset : endx], endx - self._linelen))
        
        def check_eol(self):
            assert self._linelen >= self._offset
            if self._linelen > self._offset:
                raise ValueError("Expected end of line at '%s'[%d:] == '%s' (%d chars more)" %
                                 (self._line, self._offset, self._line[self._offset:],
                                  self._linelen - self._offset))
        
        def expect_char(self, c): 
            self._check_length(1)
            if self._line[self._offset] != c:
                raise ValueError("Expected '%s' at '%s'[%d] == '%s'" %
                                 (c, self._line, self._offset, self._line[self._offset]))
            self._offset += 1
        
        def read_char(self):
            self._check_length(1)
            c = self._line[self._offset]
            self._offset += 1
            return c
        
        def skip_after_char(self, c):
            while self.read_char() != c:
                pass
        
        def read_hexb(self):
            self._check_length(2)
            off = self._offset
            try: b = int(self._line[off : off + 2], 16)
            except: raise ValueError("Expected hex byte at '%s'[%d:%d] == '%s'" %
                                     (line, off, off + 2, line[off : off + 2]))
            self._offset += 2
            return b
        
        def read_unsigned(self, size):
            assert size > 0
            self._check_length(2 * size)
            value = 0
            while size > 0:
                value = (value << 8) | self.read_hexb()
                size -= 1
            return value
        
        def read_string(self, length):
            self._check_length(length)
            s = self._line[self._offset : self._offset + length]
            self._offset += length
            return s
        
        def read_bytes(self, length):
            self._check_length(2 * length)
            return ''.join([ chr(self.read_unsigned(1)) for i in range(length) ])
    
    
    def __init__(self, lineio):
        self.__lineio = lineio
    
    def open(self):
        self.__lineio.open()
        
    def close(self):
        self.__lineio.close()
    
    def send_message(self, topic, payload):
        assert 0 < len(topic) < 256
        assert len(payload) < 256
        now = int(time.time()) & 0xFFFFFFFF
        cs = Checksummer()
        cs.add_uint(now)
        cs.add_uint(len(topic))
        cs.add_bytes(topic)
        cs.add_uint(len(payload))
        cs.add_bytes(payload)
        args = (now, len(topic), topic, len(payload), str2hexb(payload), cs.compute_checksum())
        line = '@%.8X:%.2X%s:%.2X%s:%0.2X' % args
        self.__lineio.writeln(line)
    
    def send_advertisement(self, topic):
        assert 0 < len(topic) < 256
        assert 0 < len(_MODULE_NAME) <= 7
        now = int(time.time()) & 0xFFFFFFFF
        cs = Checksummer()
        cs.add_uint(now)
        cs.add_uint(len(_MODULE_NAME))
        cs.add_bytes(_MODULE_NAME)
        cs.add_uint(len(topic))
        cs.add_bytes(topic)
        args = (now, len(_MODULE_NAME), _MODULE_NAME, len(topic), topic, cs.compute_checksum())
        line = '@%.8X:00:p:%.2X%s:%.2X%s:%0.2X' % args
        self.__lineio.writeln(line)
    
    def send_subscription(self, topic, queue_length):
        assert 0 < len(topic) < 256
        assert 0 < queue_length < 256
        assert 0 < len(_MODULE_NAME) <= 7
        now = int(time.time()) & 0xFFFFFFFF
        cs = Checksummer()
        cs.add_uint(now)
        cs.add_uint(queue_length)
        cs.add_uint(len(_MODULE_NAME))
        cs.add_bytes(_MODULE_NAME)
        args = (now, queue_length, len(_MODULE_NAME), _MODULE_NAME, len(topic), topic, cs.compute_checksum())
        line = '@%.8X:00:s%.2X:%.2X%s:%.2X%s:%0.2X' % args
        self.__lineio.writeln(line)
    
    def send_stop(self):
        now = int(time.time()) & 0xFFFFFFFF
        cs = Checksummer()
        cs.add_uint(now)
        cs.add_bytes('t')
        line = '@%.8X:00:t:%0.2X' % (now, cs.compute_checksum())
        self.__lineio.writeln(line)
    
    def send_reboot(self):
        now = int(time.time()) & 0xFFFFFFFF
        cs = Checksummer()
        cs.add_uint(now)
        cs.add_bytes('r')
        line = '@%.8X:00:r:%0.2X' % (now, cs.compute_checksum())
        self.__lineio.writeln(line)
        
    def recv(self):
        line = self.__lineio.readln()
        parser = self.MsgParser(line)
        cs = Checksummer()
        
        # Receive the incoming message
        parser.expect_char('@')
        deadline = parser.read_unsigned(4)
        cs.add_uint(deadline)
        
        parser.expect_char(':')
        length = parser.read_unsigned(1)
        topic = parser.read_string(length)
        cs.add_uint(length)
        cs.add_bytes(topic)
        
        parser.expect_char(':')
        if length > 0: # Normal message
            length = parser.read_unsigned(1)
            payload = parser.read_bytes(length)
            cs.add_uint(length)
            cs.add_bytes(payload)
            
            parser.expect_char(':')
            checksum = parser.read_unsigned(1)
            cs.check(checksum)
            
            parser.check_eol()
            return (Transport.TypeEnum.MESSAGE, topic, payload)
        
        else: # Management message
            typechar = parser.read_char().lower()
            cs.add_uint(ord(typechar))
            
            if typechar == 'p':
                parser.expect_char(':')
                length = parser.read_unsigned(1)
                module = parser.read_string(length)
                cs.add_uint(length)
                cs.add_bytes(module)
                
                parser.expect_char(':')
                length = parser.read_unsigned(1)
                topic = parser.read_string(length)
                cs.add_uint(length)
                cs.add_bytes(topic)
                
                parser.expect_char(':')
                checksum = parser.read_unsigned(1)
                cs.check(checksum)
                
                parser.check_eol()
                return (Transport.TypeEnum.ADVERTISEMENT, topic)
            
            elif typechar == 's':
                queue_length = parser.read_unsigned(1)
                cs.add_uint(queue_length)
                
                parser.expect_char(':')
                length = parser.read_unsigned(1)
                module = parser.read_string(length)
                cs.add_uint(length)
                cs.add_bytes(module)
                
                parser.expect_char(':')
                length = parser.read_unsigned(1)
                topic = parser.read_string(length)
                cs.add_uint(length)
                cs.add_bytes(topic)
                
                parser.expect_char(':')
                checksum = parser.read_unsigned(1)
                cs.check(checksum)
                
                parser.check_eol()
                return (Transport.TypeEnum.SUBSCRIPTION, topic, queue_length)
            
            elif typechar == 't':
                parser.expect_char(':')
                checksum = parser.read_unsigned(1)
                cs.check(checksum)
                
                parser.check_eol()
                return (Transport.TypeEnum.STOP,)
            
            elif typechar == 'r':
                parser.expect_char(':')
                checksum = parser.read_unsigned(1)
                cs.check(checksum)
                
                parser.check_eol()
                return (Transport.TypeEnum.RESET,)
            
            else:
                raise ValueError("Unknown management message type '%s'" % typechar)

#==============================================================================

class IhexRecord(Serializable):
    MAX_DATA_LENGTH = 16
    
    class TypeEnum:
        DATA                        = 0
        END_OF_FILE                 = 1
        EXTENDED_SEGMENT_ADDRESS    = 2
        START_SEGMENT_ADDRESS       = 3
        EXTENDED_LINEAR_ADDRESS     = 4
        START_LINEAR_ADDRESS        = 5
    
    def __init__(self):
        self.count = 0
        self.offset = 0
        self.type = -1
        self.data = ''
        self.checksum = 0
        
    def __repr__(self):
        return str(self.__dict__)
        
    def __str__(self):
        return ':%0.2X%0.4X%0.2X%s%0.2X' % \
            (self.count, self.offset, self.type, str2hexb(self.data), self.checksum)

    def compute_checksum(self):
        cs = Checksummer()
        cs.add_uint(self.count)
        cs.add_uint(self.offset)
        cs.add_uint(self.type)
        cs.add_bytes(self.data)
        return cs.compute_checksum()
    
    def check_valid(self):
        if not (0 <= self.count <= 255):
            raise ValueError('not(0 <= count=%d <= 255)' % self.count)
        if self.count != len(self.data):
            raise ValueError('count=%d != len(data)=%d' % (self.count, len(self.data)))
        if not (0 <= self.offset <= 0xFFFF):
            raise ValueError('not(0 <= offset=0x%0.8X <= 0xFFFF)' % self.offset)
        if self.checksum != self.compute_checksum():
            raise ValueError('checksum=%d != expected=%d' % (self.checksum, self.compute_checksum()));
        
        if self.type == self.TypeEnum.DATA:
            pass
        elif self.type == self.TypeEnum.END_OF_FILE:
            if self.count  != 0: raise ValueError('count=%s != 0' % self.count)
        elif self.type == self.TypeEnum.EXTENDED_SEGMENT_ADDRESS:
            if self.count  != 2: raise ValueError('count=%s != 2' % self.count)
            if self.offset != 0: raise ValueError('offset=%s != 0' % self.count)
        elif self.type == self.TypeEnum.START_SEGMENT_ADDRESS:
            if self.count  != 4: raise ValueError('count=%s != 4' % self.count)
            if self.offset != 0: raise ValueError('offset=%s != 0' % self.count)
        elif self.type == self.TypeEnum.EXTENDED_LINEAR_ADDRESS:
            if self.count  != 2: raise ValueError('count=%s != 2' % self.count)
            if self.offset != 0: raise ValueError('offset=%s != 0' % self.count)
        elif self.type == self.TypeEnum.START_LINEAR_ADDRESS:
            if self.count  != 4: raise ValueError('count=%s != 4' % self.count)
            if self.offset != 0: raise ValueError('offset=%s != 0' % self.count)
        else:
            raise ValueError('Unknown type %s' % self.type)
    
    def marshal(self):
        return struct.pack('<BHB%dsB' % self.MAX_DATA_LENGTH,
                           self.count, self.offset, self.type, self.data, self.checksum)
    
    def unmarshal(self, data):
        self.count, self.offset, self.type, self.data, self.checksum = \
            struct.unpack_from('<BHB%dsB' % self.MAX_DATA_LENGTH, data)
        self.data = self.data[:self.count]
    
    def parse_ihex(self, entry):
        if entry[0] != ':': raise ValueError("Entry '%s' does not start with ':'" % entry)
        self.count = int(entry[1:3], 16)
        explen = 1 + 2 * (1 + 2 + 1 + self.count + 1)
        if len(entry) < explen:
            raise ValueError("len('%s') < %d" % (entry, explen))
        self.offset = int(entry[3:7], 16)
        self.type = int(entry[7:9], 16)
        entry = entry[9:]
        self.data = str(bytearray([ int(entry[i : (i + 2)], 16) for i in range(0, 2 * self.count, 2) ]))
        self.checksum = int(entry[(2 * self.count) : (2 * self.count + 2)], 16)
        self.check_valid()
        return self

#==============================================================================

class BootMsg(Serializable):
    MAX_PAYLOAD_LENGTH  = 30
    MAX_LENGTH          = 30 + 1 
    
    class TypeEnum:
        NACK                =  0
        ACK                 =  1
        BEGIN_LOADER        =  2
        END_LOADER          =  3
        LINKING_SETUP       =  4
        LINKING_ADDRESSES   =  5
        LINKING_OUTCOME     =  6
        IHEX_RECORD         =  7
        REMOVE_LAST         =  8
        REMOVE_ALL          =  9
        BEGIN_APPINFO       = 10
        END_APPINFO         = 11
        APPINFO_SUMMARY     = 12
        BEGIN_SETPARAM      = 13
        END_SETPARAM        = 14
        BEGIN_GETPARAM      = 15
        END_GETPARAM        = 16
        PARAM_REQUEST       = 17
        PARAM_CHUNK         = 18
    
    class ErrorInfo(Serializable):
        MAX_TEXT_LENGTH = 20
        
        class ReasonEnum:
            UNKNOWN         = 0
            NO_FREE_MEMORY  = 1
            ZERO_LENGTH     = 2
            OUT_OF_RANGE    = 3
        
        class TypeEnum:
            NONE            = 0
            TEXT            = 1
            INTEGRAL        = 2
            UINTEGRAL       = 3
            ADDRESS         = 4
            LENGTH          = 5
            CHUNK           = 6
        
        def __init__(self):
            self.line = 0
            self.reason = 0
            self.type = 0
            self.text = ''
            self.integral = 0
            self.uintegral = 0
            self.address = 0
            self.length = 0
        
        def __repr__(self):
            e = self.TypeEnum
            if   self.type == e.NONE:       value = 'empty'
            elif self.type == e.TEXT:       value = 'text="%s"' % self.text
            elif self.type == e.INTEGRAL:   value = 'integral=%d' % self.integral
            elif self.type == e.UINTEGRAL:  value = 'uintegral=%d' % self.uintegral
            elif self.type == e.ADDRESS:    value = 'address=0x%0.8X' % self.address
            elif self.type == e.LENGTH:     value = 'length=0x%0.8X' % self.length
            elif self.type == e.CHUNK:      value = 'chunk(address=%d, length=%d)' % (self.address, self.length)
            else: raise ValueError('Unknown error type %d' % self.type)
            return 'ErrorInfo(line=%d, reason=%d, %s)' % (self.line, self.reason, value)
        
        def marshal(self):
            bytes = struct.pack('<HBB', self.line, self.reason, self.type)
            e = self.TypeEnum
            if   self.type == e.NONE:       pass
            elif self.type == e.TEXT:       bytes += struct.pack('%ds' % self.MAX_TEXT_LENGTH)
            elif self.type == e.INTEGRAL:   bytes += struct.pack('<l', self.integral)
            elif self.type == e.UINTEGRAL:  bytes += struct.pack('<L', self.uintegral)
            elif self.type == e.ADDRESS:    bytes += struct.pack('<L', self.address)
            elif self.type == e.LENGTH:     bytes += struct.pack('<L', self.length)
            elif self.type == e.CHUNK:      bytes += struct.pack('<LL', self.address, self.length)
            else: raise ValueError('Unknown error type %d' % self.type)
            return bytes
            
        def unmarshal(self, data):
            self.__init__()
            self.line, self.reason, self.type = struct.unpack_from('<HBB', data)
            e = self.TypeEnum
            if   self.type == e.NONE:       pass
            elif self.type == e.TEXT:       self.text = struct.unpack_from('%ds' % self.MAX_TEXT_LENGTH, data)
            elif self.type == e.INTEGRAL:   self.integral = struct.unpack_from('<l', data)
            elif self.type == e.UINTEGRAL:  self.uintegral = struct.unpack_from('<L', data)
            elif self.type == e.ADDRESS:    self.address = struct.unpack_from('<L', data)
            elif self.type == e.LENGTH:     self.length = struct.unpack_from('<L', data)
            elif self.type == e.CHUNK:      self.address, self.length = struct.unpack_from('<LL', data)
            else: raise ValueError('Unknown error type %d' % self.type)
    
    class LinkingSetup(Serializable):
        class FlagsEnum:
            ENABLED = (1 << 0)
        
        def __init__(self):
            self.pgmlen = 0
            self.bsslen = 0
            self.datalen = 0
            self.stacklen = 0
            self.name = ''
            self.flags = self.FlagsEnum.ENABLED
        
        def marshal(self):
            return struct.pack('<LLLL%dsH' % NODE_NAME_MAX_LENGTH, self.pgmlen, self.bsslen,
                               self.datalen, self.stacklen, self.name, self.flags)
            
        def unmarshal(self, data):
            self.__init__()
            self.pgmlen, self.bsslen, self.datalen, self.stacklen, self.name, self.flags = \
                struct.unpack_from('<LLLL%dsH' % NODE_NAME_MAX_LENGTH, data)

    class LinkingAddresses(Serializable):
        def __init__(self):
            self.infoadr = 0
            self.pgmadr = 0
            self.bssadr = 0
            self.dataadr = 0
            self.datapgmadr = 0
            self.nextadr = 0
        
        def marshal(self):
            return struct.pack('<LLLLLL', self.infoadr, self.pgmadr, self.bssadr,
                               self.dataadr, self.datapgmadr, self.nextadr)
        
        def unmarshal(self, data):
            self.__init__()
            self.infoadr, self.pgmadr, self.bssadr, self.dataadr, self.datapgmadr, self.nextadr = \
                struct.unpack_from('<LLLLLL', data)

    class LinkingOutcome(Serializable):
        def __init__(self):
            self.mainadr = 0
            self.cfgadr = 0
            self.cfglen = 0
            self.ctorsadr = 0
            self.ctorslen = 0
            self.dtorsadr = 0
            self.dtorslen = 0
            
        def marshal(self):
            return struct.pack('<LLLLLLL', self.mainadr, self.cfgadr, self.cfglen,
                               self.ctorsadr, self.ctorslen, self.dtorsadr, self.dtorslen)

        def unmarshal(self, data):
            self.__init__()
            self.mainadr, self.cfgadr, self.cfglen, self.ctorsadr, self.dtorslen, self.dtorsadr, self.dtorslen = \
              struct.unpack_from('<LLLLLLL', data)

    class AppInfoSummary(Serializable):
        def __init__(self):
            self.numapps = 0
            self.freeadr = 0
            self.pgmstartadr = 0
            self.pgmendadr = 0
            self.ramstartadr = 0
            self.ramendadr = 0
        
        def marshal(self):
            return struct.pack('<LLLLLL', self.numapps, self.freeadr,
                               self.pgmstartadr, self.pgmendadr,
                               self.ramstartadr, self.ramendadr)
        
        def unmarshal(self, data):
            self.__init__()
            self.numapps, self.freeadr, self.pgmstartadr, self.pgmendadr, \
            self.ramstartadr, self.ramendadr, \
                = struct.unpack_from('<LLLLLL', data)

    class ParamRequest(Serializable):
        def __init__(self):
            self.offset = 0
            self.appname = ''
            self.length = 0
        
        def marshal(self):
            return struct.pack('<L%dsB' % NODE_NAME_MAX_LENGTH,
                               self.offset, self.appname, self.length)
        
        def unmarshal(self, data):
            self.__init__()
            self.offset, self.appname, self.length = \
                struct.unpack_from('<L%dsB' % NODE_NAME_MAX_LENGTH, data)

    class ParamChunk(Serializable):
        MAX_DATA_LENGTH = 16

        def __init__(self):
            self.data = ''
            
        def marshal(self):
            assert len(self.data) <= self.MAX_DATA_LENGTH
            return self.data

        def unmarshal(self, data):
            self.__init__()
            self.data = data[:self.MAX_DATA_LENGTH]


    def __init__(self, type=None, *args, **kwargs):
        self.type = -1
        # TODO: Initialize all types to None, and build only when needed
        self.ihex_record = IhexRecord()
        self.error_info = self.ErrorInfo()
        self.linking_setup = self.LinkingSetup()
        self.linking_addresses = self.LinkingAddresses()
        self.linking_outcome = self.LinkingOutcome()
        self.appinfo_summary = self.AppInfoSummary()
        self.param_request = self.ParamRequest()
        self.param_chunk = self.ParamChunk()
        
        if not type is None:
            self.clean(type)
            e = BootMsg.TypeEnum
            if   type == e.NACK:              self.set_error_info(*args, **kwargs)
            elif type == e.LINKING_SETUP:     self.set_linking_setup(*args, **kwargs)
            elif type == e.LINKING_ADDRESSES: self.set_linking_addresses(*args, **kwargs)
            elif type == e.LINKING_OUTCOME:   self.set_linking_outcome(*args, **kwargs)
            elif type == e.IHEX_RECORD:       self.set_ihex_record(*args, **kwargs)
            elif type == e.APPINFO_SUMMARY:   self.set_appinfo_summary(*args, **kwargs)
            elif type == e.PARAM_REQUEST:     self.set_param_request(*args, **kwargs)
            elif type == e.PARAM_CHUNK:       self.set_param_chunk(*args, **kwargs)
            else: raise ValueError('Unknown bootloader message subtype %d' % type)
    
    def __repr__(self):
        return str(self)
        
    def __str__(self):
        return str2hexb(self.marshal())
    
    def check_type(self, type):
        assert not type is None
        if self.type != type:
            raise ValueError('Unknown bootloader message subtype %d' % type)
    
    def clean(self, type=None):
        self.__init__()
        self.type = type # TODO: Build only the needed type
    
    def set_error_info(self, line, reason, type, *args, **kwargs):
        self.clean(BootMsg.TypeEnum.NACK)
        self.error_info.line = line
        self.error_info.reason = reason
        self.error_info.type = type
    
    def set_linking_setup(self, pgmlen, bsslen, datalen, stacklen, name, flags):
        self.clean(BootMsg.TypeEnum.LINKING_SETUP)
        self.linking_setup.pgmlen = pgmlen
        self.linking_setup.bsslen = bsslen
        self.linking_setup.datalen = datalen
        self.linking_setup.stacklen = stacklen
        self.linking_setup.name = name
        self.linking_setup.flags = flags
    
    def set_linking_addresses(self, infoadr, pgmadr, bssadr, dataadr, datapgmadr, nextadr):
        self.clean(BootMsg.TypeEnum.LINKING_ADDRESSES)
        self.linking_addresses.infoadr = infoadr
        self.linking_addresses.pgmadr = pgmadr
        self.linking_addresses.bssadr = bssadr
        self.linking_addresses.dataadr = dataadr
        self.linking_addresses.datapgmadr = datapgmadr
        self.linking_addresses.nextadr = nextadr
    
    def set_linking_outcome(self, mainadr, cfgadr, cfglen, ctorsadr, ctorslen, dtorsadr, dtorslen):
        self.clean(BootMsg.TypeEnum.LINKING_OUTCOME)
        self.linking_outcome.mainadr = mainadr
        self.linking_outcome.cfgadr = cfgadr
        self.linking_outcome.cfglen = cfglen
        self.linking_outcome.ctorsadr = ctorsadr
        self.linking_outcome.ctorslen = ctorslen
        self.linking_outcome.dtorsadr = dtorsadr
        self.linking_outcome.dtorslen = dtorslen
    
    def set_appinfo_summary(self, numapps, freeadr, pgmstartadr, pgmendadr, ramstartadr, ramendadr):
        self.clean(BootMsg.TypeEnum.APPINFO_SUMMARY)
        self.appinfo_summary.numapps = numapps
        self.appinfo_summary.freeadr = freeadr
        self.appinfo_summary.pgmstartadr = pgmstartadr
        self.appinfo_summary.pgmendadr = pgmendadr
        self.appinfo_summary.ramstartadr = ramstartadr
        self.appinfo_summary.ramendadr = ramendadr
        
    def set_param_request(self, offset, appname, length):
        self.clean(BootMsg.TypeEnum.PARAM_REQUEST)
        self.param_request.offset = offset
        self.param_request.appname = appname
        self.param_request.length = length
    
    def set_ihex(self, ihex_record):
        self.clean(BootMsg.TypeEnum.IHEX_RECORD)
        self.ihex_record = ihex_record
    
    def marshal(self):
        t = self.type
        e = BootMsg.TypeEnum
        if t in (e.ACK, e.REMOVE_LAST, e.REMOVE_ALL, e.BEGIN_LOADER, e.END_LOADER,
                 e.BEGIN_APPINFO, e.END_APPINFO, e.BEGIN_SETPARAM, e.END_SETPARAM,
                 e.BEGIN_GETPARAM, e.END_GETPARAM):
            payload = ''
        elif t == e.NACK:               payload = self.error_info.marshal()
        elif t == e.LINKING_SETUP:      payload = self.linking_setup.marshal()
        elif t == e.LINKING_ADDRESSES:  payload = self.linking_addresses.marshal()
        elif t == e.LINKING_OUTCOME:    payload = self.linking_outcome.marshal()
        elif t == e.IHEX_RECORD:        payload = self.ihex_record.marshal()
        elif t == e.APPINFO_SUMMARY:    payload = self.appinfo_summary.marshal()
        elif t == e.PARAM_REQUEST:      payload = self.param_request.marshal()
        elif t == e.PARAM_CHUNK:        payload = self.param_chunk.marshal()
        else: raise ValueError('Unknown bootloader message subtype %d' % self.type)
        return struct.pack('<%dsB' % self.MAX_PAYLOAD_LENGTH, payload, self.type)
    
    def unmarshal(self, data):
        self.clean()
        if len(data) < self.MAX_LENGTH:
            raise ValueError("len('%s')=%d < %d" % (data, len(data), self.MAX_LENGTH))
        payload, self.type = struct.unpack_from('<%dsB' % self.MAX_PAYLOAD_LENGTH, data)
        
        t = self.type
        e = BootMsg.TypeEnum
        if t in (e.ACK, e.REMOVE_LAST, e.REMOVE_ALL, e.BEGIN_LOADER, e.END_LOADER,
                 e.BEGIN_APPINFO, e.END_APPINFO, e.BEGIN_SETPARAM, e.END_SETPARAM,
                 e.BEGIN_GETPARAM, e.END_GETPARAM):
            pass
        elif t == e.NACK:               self.error_info.unmarshal(payload)
        elif t == e.LINKING_SETUP:      self.linking_setup.unmarshal(payload)
        elif t == e.LINKING_ADDRESSES:  self.linking_addresses.unmarshal(payload)
        elif t == e.LINKING_OUTCOME:    self.linking_outcome.unmarshal(payload)
        elif t == e.IHEX_RECORD:        self.ihex_record.unmarshal(payload)
        elif t == e.APPINFO_SUMMARY:    self.appinfo_summary.unmarshal(payload)
        elif t == e.PARAM_REQUEST:      self.param_request.unmarshal(payload)
        elif t == e.PARAM_CHUNK:        self.param_chunk.unmarshal(payload)
        else: raise ValueError('Unknown bootloader message subtype %d' % self.type)

#==============================================================================

class MgmtMsg(Serializable):
    class TypeEnum:
        RAW                     = 0x00
    
        INFO_MODULE             = 0x10
        INFO_ADVERTISEMENT      = 0x11
        INFO_SUBSCRIPTION       = 0x12
    
        CMD_GET_NETWORK_STATE   = 0x20
        CMD_ADVERTISE           = 0x21
        CMD_SUBSCRIBE           = 0x22
    
    
    def __init__(self):
        self.type = -1
        
        # TODO ...

#==============================================================================

class Bootloader:
    def __init__(self, transport, boot_topic):
        self.__transport = transport
        self.__boot_topic = boot_topic
    
    def stop(self):
        # Stop all middlewares
        logging.info('Stopping all middlewares')
        self.__transport.send_stop()
        
        logging.info('Awaiting target bootloader with topic "%s"' % self.__boot_topic)
        nack = BootMsg()
        nack.type = BootMsg.TypeEnum.NACK
        
        while True:
            try:
                self.__publish(nack)
                if self.__fetch().type == BootMsg.TypeEnum.NACK:
                    break
            except:
                logging.debug('Ignoring last received message (broken from previous communication?)')
            self.__publish(nack)
            if self.__fetch().type == BootMsg.TypeEnum.NACK:
                break
            logging.debug('NACK sent but not received, trying again')
            
        logging.info('Target bootloader alive')
    
    def reboot(self):
        # Reboot the board
        logging.info('Sending reboot signal')
        self.__transport.send_reboot()
        logging.info('Reboot signal sent')

    def load(self, appname, hexpath, stacklen, ldexe, ldgcc, ldscript, ldoself, ldmap, ldobjelf, ldobjs, *args, **kwargs):
        self.__do_wrapper(self.__do_load, appname, hexpath, stacklen, ldexe, ldgcc, ldscript, ldoself, ldmap, ldobjelf, ldobjs, *args, **kwargs)
    
    def get_appinfo(self):
        self.__do_wrapper(self.__do_get_appinfo)
    
    def set_parameter(self, appname, offset, value):
        self.__do_wrapper(self.__do_set_parameter, appname, offset, value)
        
    def get_parameter(self, appname, offset, length):
        return self.__do_wrapper(self.__do_get_parameter, appname, offset, length)
    
    def remove_last(self):
        self.__do_wrapper(self.__do_remove_last)
    
    def remove_all(self):
        self.__do_wrapper(self.__do_remove_all)
    
    def __abort(self):
        try:
            logging.warning('Aborting current bootloader procedure')
            nack = BootMsg()
            nack.clean(BootMsg.TypeEnum.NACK)
            
            while True:
                self.__publish(nack)
                response = self.__fetch()
                if response.type == BootMsg.TypeEnum.NACK:
                    break
            
            logging.warning('Procedure aborted')
        except:
            pass
    
    def __publish(self, msg):
        self.__transport.send_message(self.__boot_topic, msg.marshal())
    
    def __fetch(self, expected_type=None):
        while True:
            fields = self.__transport.recv()
            if fields[0] == Transport.TypeEnum.MESSAGE and fields[1] == self.__boot_topic:
                msg = BootMsg()
                msg.unmarshal(fields[2])
                if msg.type == BootMsg.TypeEnum.NACK:
                    logging.debug(repr(msg.error_info))
                if expected_type != None and msg.type != expected_type:
                    raise ValueError('msg.type=%d != %d' % (msg.type, expected_type))
                return msg
    
    def __do_wrapper(self, do_function, *args, **kwargs):
        try:
            return do_function(*args, **kwargs)
        
        except KeyboardInterrupt:
            try:
                self.__abort()
            except KeyboardInterrupt:
                pass
        
        except Exception as e:
            logging.error(str(e))
            self.__abort()
            raise
    
    
    def __do_load(self, appname, hexpath, stacklen, ldexe, ldgcc, ldscript, ldoself, ldmap, ldobjelf, ldobjs, *args, **kwargs):
        publish = self.__publish
        fetch = self.__fetch
        msg = BootMsg()
        
        # Begin loading
        logging.info('Initiating bootloading procedure')
        msg.clean(BootMsg.TypeEnum.BEGIN_LOADER)
        publish(msg)
        fetch(BootMsg.TypeEnum.ACK)
        
        # Get the length of each section
        logging.info('Reading section lengths from "%s"' % ldobjelf)
        pgmlen, bsslen, datalen = 0, 0, 0
        with open(ldobjelf, 'rb') as f:
            elffile = ELFFile(f)
            
            text_start  = _get_symbol_address(elffile, '__text_start__')
            text_end    = _get_symbol_address(elffile, '__text_end__')
            pgmlen = text_end - text_start
            
            bss_start   = _get_symbol_address(elffile, '__bss_start__')
            bss_end     = _get_symbol_address(elffile, '__bss_end__')
            bsslen = bss_end - bss_start
            
            data_start  = _get_symbol_address(elffile, '__data_start__')
            data_end    = _get_symbol_address(elffile, '__data_end__')
            datalen = data_end - data_start
        
        appflags = BootMsg.LinkingSetup.FlagsEnum.ENABLED
        logging.info('  pgmlen   = 0x%0.8X (%d)' % (pgmlen, pgmlen))
        logging.info('  bsslen   = 0x%0.8X (%d)' % (bsslen, bsslen))
        logging.info('  datalen  = 0x%0.8X (%d)' % (datalen, datalen))
        logging.info('  stacklen = 0x%0.8X (%d)' % (stacklen, stacklen))
        logging.info('  appname  = "%s"' % appname)
        logging.info('  flags    = 0x%0.4X' % appflags)
        
        # Send the section sizes and app name
        logging.info('Sending section lengths to target module')
        msg.set_linking_setup(pgmlen, bsslen, datalen, stacklen, appname, appflags)
        linking_setup = msg.linking_setup
        publish(msg)
        
        # Receive the allocated addresses
        logging.info('Receiving the allocated addresses from target module')
        msg = fetch(BootMsg.TypeEnum.LINKING_ADDRESSES)
        linking_addresses = msg.linking_addresses
        pgmadr     = linking_addresses.pgmadr
        bssadr     = linking_addresses.bssadr
        dataadr    = linking_addresses.dataadr
        datapgmadr = linking_addresses.datapgmadr
        
        logging.info('  pgmadr     = 0x%0.8X' % pgmadr)
        logging.info('  bssadr     = 0x%0.8X' % bssadr)
        logging.info('  dataadr    = 0x%0.8X' % dataadr)
        logging.info('  datapgmadr = 0x%0.8X' % datapgmadr)
        
        # Link to the OS symbols
        args = [
            '--script', ldscript,
            '--just-symbols', ldoself,
            '-o', ldobjelf,
            '--section-start', '.text=0x%0.8X' % pgmadr
        ]
        if bssadr:
            args += [ '--section-start', '.bss=0x%0.8X' % bssadr ]
        if dataadr:
            args += [ '--section-start', '.data=0x%0.8X' % dataadr ]
        if ldmap:
            args += [ '-Map', ldmap ]
        if ldgcc:
            args = [ '-Wl,' + ','.join(args) ]
        args = [ ldexe ] + args + list(ldobjs)
        logging.debug('Linker command line:')
        logging.debug('  ' + ' '.join([ arg for arg in args ]))
        
        logging.info('Linking object files')
        if 0 != subprocess.call(args):
            raise RuntimeError("Cannot link '%s' with '%s'" % (ldoself, ldobjelf))
        
        logging.info('Generating executable')
        if 0 != subprocess.call(['make']):
            raise RuntimeError("Cannot make the application binary")
        
        logging.info('Reading final ELF file "%s"' % ldobjelf)
        with open(ldobjelf, 'rb') as f:
            elffile = ELFFile(f)
            
            mainadr = _get_function_address(elffile, APP_THREAD_SYMBOL)
            logging.info('  mainadr   = 0x%0.8X' % mainadr)
            
            config_start  = _get_symbol_address(elffile, '__config_start__')
            config_end    = _get_symbol_address(elffile, '__config_end__')
            cfgadr = config_start
            cfglen = config_end - config_start
            logging.info('  cfgadr    = 0x%0.8X' % cfgadr)
            logging.info('  cfglen    = 0x%0.8X (%d)' % (cfglen, cfglen))
            
            try:
                ctorsadr = _get_symbol_address(elffile, '__init_array_start')
                ctorslen = (_get_symbol_address(elffile, '__init_array_end') - ctorsadr) / 4
            except RuntimeError:
                logging.debug('Section "constructors" looks empty')
                ctorsadr = 0
                ctorslen = 0
            logging.info('  ctorsadr  = 0x%0.8X' % ctorsadr)
            logging.info('  ctorslen  = 0x%0.8X (%d)' % (ctorslen, ctorslen))
            
            try:
                dtorsadr = _get_symbol_address(elffile, '__fini_array_start')
                dtorslen = (_get_symbol_address(elffile, '__fini_array_end') - dtorsadr) / 4
            except RuntimeError:
                logging.debug('Section "destructors" looks empty')
                dtorsadr = 0
                dtorslen = 0
            logging.info('  dtorsadr  = 0x%0.8X' % dtorsadr)
            logging.info('  dtorslen  = 0x%0.8X (%d)' % (dtorslen, dtorslen))
        
        # Send the linking outcome
        msg.set_linking_outcome(mainadr, cfgadr, cfglen, ctorsadr, ctorslen, dtorsadr, dtorslen)
        publish(msg)
        fetch(BootMsg.TypeEnum.ACK)
        
        # Read the generated IHEX file and remove unused records
        logging.info('Reading IHEX file')
        with open(hexpath, 'r') as f:
            hexdata = f.readlines()
        
        logging.info('Removing unused IHEX records')
        ihex_records = []
        for line in hexdata:
            line = line.strip()
            if len(line) == 0: continue
            record = IhexRecord()
            record.parse_ihex(line)
            if record.type == IhexRecord.TypeEnum.DATA:
                # Split into smaller packets
                while record.count > IhexRecord.MAX_DATA_LENGTH:
                    bigrecord = IhexRecord()
                    bigrecord.count = IhexRecord.MAX_DATA_LENGTH
                    bigrecord.offset = record.offset
                    bigrecord.type = IhexRecord.TypeEnum.DATA
                    bigrecord.data = record.data[:IhexRecord.MAX_DATA_LENGTH]
                    ihex_records.append(bigrecord)
                    
                    record.count -= IhexRecord.MAX_DATA_LENGTH
                    record.offset += IhexRecord.MAX_DATA_LENGTH
                    record.data = record.data[IhexRecord.MAX_DATA_LENGTH:]
                
                ihex_records.append(record)
                
            elif record.type != IhexRecord.TypeEnum.START_SEGMENT_ADDRESS and \
                 record.type != IhexRecord.TypeEnum.START_LINEAR_ADDRESS:
                ihex_records.append(record)
            else:
                logging.info('  ' + str(record))
        
        # Send IHEX records
        logging.info('Sending IHEX records to target module')
        for i in range(len(ihex_records)):
            record = ihex_records[i]
            logging.info('  %s (%d/%d)' % (str(record), i + 1, len(ihex_records)))
            msg.set_ihex(record)
            publish(msg)
            fetch(BootMsg.TypeEnum.ACK)
        
        # End loading
        logging.info('Finalizing bootloading procedure')
        msg.clean(BootMsg.TypeEnum.END_LOADER)
        publish(msg)
        fetch(BootMsg.TypeEnum.ACK)
        
        logging.info('Bootloading completed successfully')
        return True


    def __do_get_appinfo(self):
        publish = self.__publish
        fetch = self.__fetch
        msg = BootMsg()
        
        # Begin appinfo retrieval
        logging.info('Retrieving appinfo')
        msg.clean(BootMsg.TypeEnum.BEGIN_APPINFO)
        publish(msg)
        summary = fetch(BootMsg.TypeEnum.APPINFO_SUMMARY).appinfo_summary
        logging.info('  numapps     = %d' % summary.numapps)
        logging.info('  freeadr     = 0x%0.8X' % summary.freeadr)
        logging.info('  pgmstartadr = 0x%0.8X' % summary.pgmstartadr)
        logging.info('  pgmendadr   = 0x%0.8X' % summary.pgmendadr)
        logging.info('  ramstartadr = 0x%0.8X' % summary.ramstartadr)
        logging.info('  ramendadr   = 0x%0.8X' % summary.ramendadr)
        
        msg.clean(BootMsg.TypeEnum.ACK)
        for i in range(summary.numapps):
            # Get the linking setup
            publish(msg)
            setup = fetch(BootMsg.TypeEnum.LINKING_SETUP).linking_setup
            logging.info('Found app "%s"' % setup.name)
            logging.info('  pgmlen     = 0x%0.8X (%d)' % (setup.pgmlen, setup.pgmlen))
            logging.info('  bsslen     = 0x%0.8X (%d)' % (setup.bsslen, setup.bsslen))
            logging.info('  datalen    = 0x%0.8X (%d)' % (setup.datalen, setup.datalen))
            logging.info('  stacklen   = 0x%0.8X (%d)' % (setup.stacklen, setup.stacklen))
            logging.info('  name       = "%s"' % setup.name)
            logging.info('  flags      = 0x%0.4X' % setup.flags)
            
            # Get the linking addresses
            publish(msg)
            addresses = fetch(BootMsg.TypeEnum.LINKING_ADDRESSES).linking_addresses
            logging.info('  pgmadr     = 0x%0.8X' % addresses.pgmadr)
            logging.info('  bssadr     = 0x%0.8X' % addresses.bssadr)
            logging.info('  dataadr    = 0x%0.8X' % addresses.dataadr)
            logging.info('  datapgmadr = 0x%0.8X' % addresses.datapgmadr)
            logging.info('  nextadr    = 0x%0.8X' % addresses.nextadr)
            
            # Get the linking outcome
            publish(msg)
            outcome = fetch(BootMsg.TypeEnum.LINKING_OUTCOME).linking_outcome
            logging.info('  mainadr    = 0x%0.8X' % outcome.mainadr)
            logging.info('  cfgadr     = 0x%0.8X' % outcome.cfgadr)
            logging.info('  cfglen     = 0x%0.8X (%d)' % (outcome.cfglen, outcome.cfglen))
            logging.info('  ctorsadr   = 0x%0.8X' % outcome.ctorsadr)
            logging.info('  ctorslen   = 0x%0.8X (%d)' % (outcome.ctorslen, outcome.ctorslen))
            logging.info('  dtorsadr   = 0x%0.8X' % outcome.dtorsadr)
            logging.info('  dtorslen   = 0x%0.8X (%d)' % (outcome.dtorslen, outcome.dtorslen))
        
        logging.debug('There should not be more apps')
        publish(msg)
        fetch(BootMsg.TypeEnum.END_APPINFO)
        logging.info('Ending appinfo retrieval')
        publish(msg)
        fetch(BootMsg.TypeEnum.ACK)
        logging.info('Appinfo retrieval completed successfully')
        return True
    
    
    def __do_set_parameter(self, appname, offset, value):
        publish = self.__publish
        fetch = self.__fetch
        MAX_LENGTH = BootMsg.ParamChunk.MAX_DATA_LENGTH
        msg = BootMsg()
        
        if len(value) == 0:
            raise ValueError('Willing to write an empty parameter value')
        
        # Begin parameter set
        logging.info('Beginning parameter set')
        msg.clean(BootMsg.TypeEnum.BEGIN_SETPARAM)
        publish(msg)
        fetch(BootMsg.TypeEnum.ACK)
        
        # Send the parameter request
        length = len(value)
        logging.info('Sending parameter request')
        logging.info('  appname = "%s"' % appname)
        logging.info('  offset  = 0x%0.8X (%d)' % (offset, offset))
        logging.info('  length  = 0x%0.8X (%d)' % (length, length))
        msg.clean(BootMsg.TypeEnum.PARAM_REQUEST)
        msg.param_request.offset = offset
        msg.param_request.appname = appname
        msg.param_request.length = length
        publish(msg)
        fetch(BootMsg.TypeEnum.ACK)
        
        # Send each parameter chunk
        logging.info('Sending parameter chunks')
        pending = value
        while True:
            chunk = pending[:MAX_LENGTH]
            logging.info('  app_config[0x%0.8X : 0x%0.8X] = %s' %
                         (offset, offset + len(chunk), str2hexb(chunk)))
            
            msg.clean(BootMsg.TypeEnum.PARAM_CHUNK)
            msg.param_chunk.data = chunk
            publish(msg)
            fetch(BootMsg.TypeEnum.ACK)
            
            if len(chunk) >= MAX_LENGTH:
                pending = pending[MAX_LENGTH:]
                offset += MAX_LENGTH
            else:
                break
        
        # Finalize
        logging.info('Finalizing parameter')
        msg.clean(BootMsg.TypeEnum.END_SETPARAM)
        publish(msg)
        fetch(BootMsg.TypeEnum.ACK)
        logging.info('Parameter set successfully')
        return True
        
        
    def __do_get_parameter(self, appname, offset, length):
        publish = self.__publish
        fetch = self.__fetch
        MAX_LENGTH = BootMsg.ParamChunk.MAX_DATA_LENGTH
        msg = BootMsg()
        
        if length == 0:
            raise ValueError('Willing to read an empty parameter value')
        
        # Begin parameter get
        logging.info('Beginning parameter get')
        msg.clean(BootMsg.TypeEnum.BEGIN_GETPARAM)
        publish(msg)
        fetch(BootMsg.TypeEnum.ACK)
        
        # Send the parameter request
        logging.info('Sending parameter request')
        logging.info('  appname = "%s"' % appname)
        logging.info('  offset  = 0x%0.8X (%d)' % (offset, offset))
        logging.info('  length  = 0x%0.8X (%d)' % (length, length))
        msg.clean(BootMsg.TypeEnum.PARAM_REQUEST)
        msg.param_request.offset = offset
        msg.param_request.appname = appname
        msg.param_request.length = length
        publish(msg)
        
        # Receive each parameter chunk
        logging.info('Receiving parameter chunks')
        value = ''
        while len(value) < length:
            msg = fetch(BootMsg.TypeEnum.PARAM_CHUNK)
            chunk = msg.param_chunk.data
            if len(chunk) > length - len(value):
                chunk = chunk[:(length - len(value))]
            value += chunk
            logging.info('  app_config[0x%0.8X : 0x%0.8X] = %s' %
                         (offset, offset + len(chunk), str2hexb(chunk)))
            offset += len(chunk)
            msg.clean(BootMsg.TypeEnum.ACK)
            publish(msg)
            
        # Finalize
        logging.info('Finalizing parameter')
        msg.clean(BootMsg.TypeEnum.END_GETPARAM)
        publish(msg)
        fetch(BootMsg.TypeEnum.ACK)
        
        logging.info('Parameter got successfully')
        return value
    
    
    def __do_remove_last(self):
        publish = self.__publish
        fetch = self.__fetch
        msg = BootMsg()
        
        # Begin remoal
        logging.info('Initiating removal procedure')
        msg.clean(BootMsg.TypeEnum.BEGIN_LOADER)
        publish(msg)
        fetch(BootMsg.TypeEnum.ACK)
        
        # Remove all
        logging.info('Removing last app')
        msg.clean(BootMsg.TypeEnum.REMOVE_LAST)
        publish(msg)
        try:
            fetch(BootMsg.TypeEnum.ACK)
        except ValueError:
            raise RuntimeError('Cannot remove last app -- no apps installed?')
        
        # End loading
        logging.info('Finalizing removal procedure')
        msg.clean(BootMsg.TypeEnum.END_LOADER)
        publish(msg)
        fetch(BootMsg.TypeEnum.ACK)
        
        logging.info('Removal completed successfully')
        return True
    
    
    def __do_remove_all(self):
        publish = self.__publish
        fetch = self.__fetch
        msg = BootMsg()
        
        # Begin remoal
        logging.info('Initiating removal procedure')
        msg.clean(BootMsg.TypeEnum.BEGIN_LOADER)
        publish(msg)
        fetch(BootMsg.TypeEnum.ACK)
        
        # Remove all
        logging.info('Removing all apps')
        msg.clean(BootMsg.TypeEnum.REMOVE_ALL)
        publish(msg)
        fetch(BootMsg.TypeEnum.ACK)
        
        # End loading
        logging.info('Finalizing removal procedure')
        msg.clean(BootMsg.TypeEnum.END_LOADER)
        publish(msg)
        fetch(BootMsg.TypeEnum.ACK)
        
        logging.info('Removal completed successfully')
        return True
    
#==============================================================================
