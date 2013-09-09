# TODO: Check BootlaoderMsg checksums
# TODO: Add BootlaoderMsg sequence number check

import sys, os, io, time, struct, string, random
import subprocess
import serial
import logging
from elftools.elf.elffile import ELFFile


MODULE_NAME_MAX_LENGTH = 7
NODE_NAME_MAX_LENGTH = 8
TOPIC_NAME_MAX_LENGTH = 16
IHEX_MAX_DATA_LENGTH = 16

APP_THREAD_SYMBOL = 'app_thread'
THREAD_PC_OFFSET = 1
APP_CONFIG_SYMBOL = 'app_config'

_MODULE_NAME = ''.join([ random.choice(string.ascii_uppercase + string.digits)
                         for x in range(MODULE_NAME_MAX_LENGTH) ])

#==============================================================================

def _str2hexb(data):
    return ''.join([ ('%.2X' % ord(b)) for b in data ])


def _hexb2str(data):
    assert (len(data) & 1) == 0
    return ''.join([ chr(int(data[i : i + 2], 16)) for i in range(0, len(data), 2) ])
    
    
def _uintsum(value):
    value = int(value)
    assert value >= 0
    cs = 0
    while value > 0:
        cs += value & 0xFF
        value >>= 8
    return cs & 0xFF


def _get_function_address(elffile, name):
    dwarfinfo = elffile.get_dwarf_info()
    for CU in dwarfinfo.iter_CUs():
        for DIE in CU.iter_DIEs():
            try:
                if DIE.tag == 'DW_TAG_subprogram' and DIE.attributes['DW_AT_name'].value == name:
                    return int(DIE.attributes['DW_AT_low_pc'].value) + THREAD_PC_OFFSET
            except KeyError: continue
    raise RuntimeError('Symbol "%s" not found' % name)


def _get_variable_address(elffile, name):
    dwarfinfo = elffile.get_dwarf_info()
    for CU in dwarfinfo.iter_CUs():
        for DIE in CU.iter_DIEs():
            try:
                if DIE.tag == 'DW_TAG_variable' and DIE.attributes['DW_AT_name'].value == name:
                    value = DIE.attributes['DW_AT_location'].value
                    assert value[0] == 3
                    return (value[4] << 24) | (value[3] << 16) | (value[2] << 8) | value[1]
            except KeyError: continue
    raise RuntimeError('Symbol "%s" not found' % name)

#==============================================================================

class Checksummer:
    def __init__(self):
        self.__accumulator = 0
        
    def __int__(self):
        return self.compute_checksum()
        
    def compute_checksum(self):
        return 0x100 - (self.__accumulator)
    
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
        args = (now, len(topic), topic, len(payload), _str2hexb(payload), cs.compute_checksum())
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
            (self.count, self.offset, self.type, _str2hexb(self.data), self.checksum)

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
            raise ValueError('checksum=%d != expected=%d', (self.checksum, self.compute_checksum()));
        
        if self.type == self.TypeEnum.DATA:
            pass
        elif self.type == self.TypeEnum.END_OF_FILE:
            if self.count  != 0: raise ValueError('count=%s != 0', self.count)
        elif self.type == self.TypeEnum.EXTENDED_SEGMENT_ADDRESS:
            if self.count  != 2: raise ValueError('count=%s != 2', self.count)
            if self.offset != 0: raise ValueError('offset=%s != 0', self.count)
        elif self.type == self.TypeEnum.START_SEGMENT_ADDRESS:
            if self.count  != 4: raise ValueError('count=%s != 4', self.count)
            if self.offset != 0: raise ValueError('offset=%s != 0', self.count)
        elif self.type == self.TypeEnum.EXTENDED_LINEAR_ADDRESS:
            if self.count  != 2: raise ValueError('count=%s != 2', self.count)
            if self.offset != 0: raise ValueError('offset=%s != 0', self.count)
        elif self.type == self.TypeEnum.START_LINEAR_ADDRESS:
            if self.count  != 4: raise ValueError('count=%s != 4', self.count)
            if self.offset != 0: raise ValueError('offset=%s != 0', self.count)
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
    MAX_PAYLOAD_LENGTH  = 26
    MAX_LENGTH          = 26 + 1 
    
    class TypeEnum:
        NACK                =  0
        ACK                 =  1
        REMOVE_LAST         =  2
        REMOVE_ALL          =  3
        BEGIN_LOADER        =  4
        END_LOADER          =  5
        LINKING_SETUP       =  6
        LINKING_ADDRESSES   =  7
        LINKING_OUTCOME     =  8
        IHEX_RECORD         =  9
        BEGIN_APPINFO       = 10
        END_APPINFO         = 11
        APPINFO_SUMMARY     = 12
        BEGIN_SETPARAM      = 13
        END_SETPARAM        = 14
        BEGIN_GETPARAM      = 15
        END_GETPARAM        = 16
        PARAM_REQUEST       = 17
        PARAM_CHUNK         = 18
        
    class LinkingSetup(Serializable):
        def __init__(self):
            self.pgmlen = 0
            self.bsslen = 0
            self.datalen = 0
            self.stacklen = 0
            self.name = ''
        
        def marshal(self):
            return struct.pack('<LLLL%ds' % NODE_NAME_MAX_LENGTH,
                               self.pgmlen, self.bsslen, self.datalen, self.stacklen, self.name)
            
        def unmarshal(self, data):
            self.pgmlen, self.bsslen, self.datalen, self.stacklen, self.name = \
              struct.unpack_from('<LLLL%ds' % NODE_NAME_MAX_LENGTH, data)

    class LinkingAddresses(Serializable):
        def __init__(self):
            self.infoadr = 0
            self.pgmadr = 0
            self.bssadr = 0
            self.dataadr = 0
            self.datapgmadr = 0
        
        def marshal(self):
            return struct.pack('<LLLLL', self.infoadr, self.pgmadr, self.bssadr,
                               self.dataadr, self.datapgmadr)
        
        def unmarshal(self, data):
            self.infoadr, self.pgmadr, self.bssadr, self.dataadr, self.datapgmadr = \
                struct.unpack_from('<LLLLL', data)

    class LinkingOutcome(Serializable):
        def __init__(self):
            self.threadadr = 0
            self.cfgadr = 0
            self.appinfoadr = 0
            
        def marshal(self):
            return struct.pack('<LLL', self.threadadr, self.cfgadr, self.appinfoadr)

        def unmarshal(self, data):
            self.threadadr, self.cfgadr, self.appinfoadr = struct.unpack_from('<LLL', data)

    class AppInfoSummary(Serializable):
        def __init__(self):
            self.freeadr = 0
            self.headadr = 0
        
        def marshal(self):
            return struct.pack('<LL', self.freeadr, self.headadr)
        
        def unmarshal(self, data):
            self.freeadr, self.headadr = struct.unpack_from('<LL', data)

    class ParamRequest(Serializable):
        def __init__(self):
            self.offset = 0
            self.appname = ''
            self.length = 0
        
        def marshal(self):
            return struct.pack('<L%dsB' % NODE_NAME_MAX_LENGTH,
                               self.offset, self.appname, self.length)
        
        def unmarshal(self, data):
            self.offset, self.appname, self.length = \
                struct.unpack_from('<L%dsB' % NODE_NAME_MAX_LENGTH, data)

    class ParamChunk(Serializable):
        MAX_DATA_LENGTH = 16

        def __init__(self):
            self.data = ''
            
        def marshal(self):
            assert len(data) <= self.MAX_DATA_LENGTH
            return self.data

        def unmarshal(self, data):
            self.data = data[:self.MAX_DATA_LENGTH]


    def __init__(self):
        self.ihex_record = IhexRecord()
        self.linking_setup = self.LinkingSetup()
        self.linking_addresses = self.LinkingAddresses()
        self.linking_outcome = self.LinkingOutcome()
        self.appinfo_summary = self.AppInfoSummary()
        self.param_request = self.ParamRequest()
        self.param_chunk = self.ParamChunk()
        self.type = -1
    
    def __repr__(self):
        return str(self)
        
    def __str__(self):
        return _str2hexb(self.marshal())
    
    def clean(self, type=None):
        self.__init__()
        self.type = type if type != None else -1
    
    def set_linking_setup(self, pgmlen, bsslen, datalen, stacklen, name):
        self.clean(BootMsg.TypeEnum.LINKING_SETUP)
        self.linking_setup.pgmlen = pgmlen
        self.linking_setup.bsslen = bsslen
        self.linking_setup.datalen = datalen
        self.linking_setup.stacklen = stacklen
        self.linking_setup.name = name
    
    def set_linking_addresses(self, infoadr, pgmadr, bssadr, dataadr, datapgmadr):
        self.clean(BootMsg.TypeEnum.LINKING_ADDRESSES)
        self.linking_addresses.infoadr = infoadr
        self.linking_addresses.pgmadr = pgmadr
        self.linking_addresses.bssadr = bssadr
        self.linking_addresses.dataadr = dataadr
        self.linking_addresses.datapgmadr = datapgmadr
    
    def set_linking_outcome(self, threadadr, cfgadr, appinfoadr):
        self.clean(BootMsg.TypeEnum.LINKING_OUTCOME)
        self.linking_outcome.threadadr = threadadr
        self.linking_outcome.cfgadr = cfgadr
        self.linking_outcome.appinfoadr = appinfoadr
    
    def set_appinfo_summary(self, freeadr, headadr):
        self.clean(BootMsg.TypeEnum.APPINFO_SUMMARY)
        self.appinfo_summary.freeadr = freeadr
        self.appinfo_summary.headadr = headadr
        
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
        if t in (e.NACK, e.ACK, e.REMOVE_LAST, e.REMOVE_ALL, e.BEGIN_LOADER, e.END_LOADER,
                 e.BEGIN_APPINFO, e.END_APPINFO, e.BEGIN_SETPARAM, e.END_SETPARAM,
                 e.BEGIN_GETPARAM, e.END_GETPARAM):
            payload = ''
        elif t == e.LINKING_SETUP:      payload = self.linking_setup.marshal()
        elif t == e.LINKING_ADDRESSES:  payload = self.linking_addresses.marshal()
        elif t == e.LINKING_OUTCOME:    payload = self.linking_outcome.marshal()
        elif t == e.IHEX_RECORD:        payload = self.ihex_record.marshal()
        elif t == e.APPINFO_SUMMARY:    payload = self.appinfo_summary.marshal()
        elif t == e.PARAM_REQUEST:      payload = self.param_request.marshal()
        elif t == e.PARAM_CHUNK:        payload = self.param_chunk.marshal()
        else:
            raise ValueError('Unknown bootloader message subtype %d' % self.type)
        return struct.pack('<%dsB' % self.MAX_PAYLOAD_LENGTH, payload, self.type)
    
    def unmarshal(self, data):
        self.clean()
        if len(data) < self.MAX_LENGTH:
            raise ValueError("len('%s')=%d < %d" % (data, len(data), self.MAX_LENGTH))
        payload, self.type = struct.unpack_from('<%dsB' % self.MAX_PAYLOAD_LENGTH, data)
        
        t = self.type
        e = BootMsg.TypeEnum
        if t in (e.NACK, e.ACK, e.REMOVE_LAST, e.REMOVE_ALL, e.BEGIN_LOADER, e.END_LOADER,
                 e.BEGIN_APPINFO, e.END_APPINFO, e.BEGIN_SETPARAM, e.END_SETPARAM,
                 e.BEGIN_GETPARAM, e.END_GETPARAM):
            pass
        elif t == e.LINKING_SETUP:      self.linking_setup.unmarshal(payload)
        elif t == e.LINKING_ADDRESSES:  self.linking_addresses.unmarshal(payload)
        elif t == e.LINKING_OUTCOME:    self.linking_outcome.unmarshal(payload)
        elif t == e.IHEX_RECORD:        self.ihex_record.unmarshal(payload)
        elif t == e.APPINFO_SUMMARY:    self.appinfo_summary.unmarshal(payload)
        elif t == e.PARAM_REQUEST:      self.param_request.unmarshal(payload)
        elif t == e.PARAM_CHUNK:        self.param_chunk.unmarshal(payload)
        else:
            raise ValueError('Unknown bootloader message subtype %d' % self.type)

#==============================================================================

class Bootloader:
    def __init__(self, transport, boot_topic):
        self.__transport = transport
        self.__boot_topic = boot_topic
    
    def __abort(self):
        try:
            logging.warning('Aborting current bootloader procedure')
            nack = BootMsg()
            nack.type = BootMsg.TypeEnum.NACK
            
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
                if expected_type != None and msg.type != expected_type:
                    raise ValueError('msg.type=%d != %d' % (msg.type, expected_type))
                return msg
    
    def __do_wrapper(self, do_function, *args):
        try:
            return do_function(*args)
        except KeyboardInterrupt:
            self.__abort()
        except Exception as e:
            logging.error(e)
            self.__abort()
            raise
    
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

    def load(self, appname, hexpath, stacklen, ldexe, ldscript, ldoself, ldmap, ldobjelf, ldobjs):
        self.__do_wrapper(self.__do_load, appname, hexpath, stacklen, ldexe, ldscript, ldoself, ldmap, ldobjelf, ldobjs)
    
    def get_appinfo(self, appname):
        self.__do_wrapper(self.__do_get_appinfo, appname)
    
    def set_parameter(self, appname, offset, chunk):
        self.__do_wrapper(self.__do_set_parameter, appname, offset, chunk)
        
    def get_parameter(self, appname, offset, chunk):
        return self.__do_wrapper(self.__do_get_parameter, appname, offset)
    
    def __do_load(self, appname, hexpath, stacklen, ldexe, ldscript, ldoself, ldmap, ldobjelf, ldobjs):
        msg = BootMsg()
        
        # Begin loading
        logging.info('Initiating bootloading procedure')
        msg.clean(BootMsg.TypeEnum.BEGIN_LOADER)
        self.__publish(msg)
        self.__fetch(BootMsg.TypeEnum.ACK)
        
        # Get the length of each section
        logging.info('Reading section lengths from "%s"' % ldobjelf)
        pgmlen, bsslen, datalen = 0, 0, 0
        with open(ldobjelf, 'rb') as f:
            elffile = ELFFile(f)
            for section in elffile.iter_sections():
                if   section.name == '.text':   pgmlen  = int(section['sh_size'])
                elif section.name == '.bss':    bsslen  = int(section['sh_size'])
                elif section.name == '.data':   datalen = int(section['sh_size'])
        
        logging.info('  .text = 0x%0.8X (%d)' % (pgmlen, pgmlen))
        logging.info('  .bss  = 0x%0.8X (%d)' % (bsslen, bsslen))
        logging.info('  .data = 0x%0.8X (%d)' % (datalen, datalen))
        logging.info('  stacklen = 0x%0.8X (%d)' % (stacklen, stacklen))
        logging.info('  appname  = "%s"' % appname)
        
        # Send the section sizes and app name
        logging.info('Sending section lengths to target module')
        msg.set_linking_setup(pgmlen, bsslen, datalen, stacklen, appname)
        linking_setup = msg.linking_setup
        self.__publish(msg)
        
        # Receive the allocated addresses
        logging.info('Receiving the allocated addresses from target module')
        msg = self.__fetch(BootMsg.TypeEnum.LINKING_ADDRESSES)
        linking_addresses = msg.linking_addresses
        pgmadr     = linking_addresses.pgmadr
        bssadr     = linking_addresses.bssadr
        dataadr    = linking_addresses.dataadr
        datapgmadr = linking_addresses.datapgmadr
        
        logging.info('  .text = 0x%0.8X' % pgmadr)
        logging.info('  .bss  = 0x%0.8X' % bssadr)
        logging.info('  .data = 0x%0.8X' % dataadr)
        logging.info('  const = 0x%0.8X' % datapgmadr)
        
        # Link to the OS symbols
        args = [
            ldexe,
            '--script', ldscript,
            '--just-symbols', ldoself,
            '-Map', ldmap,
            '-o', ldobjelf,
            '--section-start', '.text=0x%0.8X' % pgmadr,
            '--section-start', '.bss=0x%0.8X' % bssadr,
            '--section-start', '.data=0x%0.8X' % dataadr
        ] + list(ldobjs)
        logging.info('Linker command line:')
        logging.info('  ' + ' '.join([ arg for arg in args ]))
        
        logging.info('Linking object files')
        if 0 != subprocess.call(args):
            raise RuntimeError("Cannot link '%s' with '%s'" % (ldoself, ldobjelf))
        
        logging.info('Generating executable')
        if 0 != subprocess.call(['make']):
            raise RuntimeError("Cannot make the application binary")
        
        logging.info('Reading final ELF file "%s"' % ldobjelf)
        with open(ldobjelf, 'rb') as f:
            elffile = ELFFile(f)
            threadadr = _get_function_address(elffile, APP_THREAD_SYMBOL)
            cfgadr = _get_variable_address(elffile, APP_CONFIG_SYMBOL)
            
        logging.info('  threadadr = 0x%0.8X' % threadadr)
        logging.info('  cfgadr    = 0x%0.8X' % cfgadr)
        
        # Send the linking outcome
        msg.set_linking_outcome(threadadr, cfgadr, 0) # FIXME: appinfoadr
        self.__publish(msg)
        self.__fetch(BootMsg.TypeEnum.ACK)
        
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
        for record in ihex_records:
            logging.info('  ' + str(record))
            msg.set_ihex(record)
            self.__publish(msg)
            self.__fetch(BootMsg.TypeEnum.ACK)
        
        # End loading
        logging.info('Finalizing bootloading procedure')
        msg.clean(BootMsg.TypeEnum.END_LOADER)
        self.__publish(msg)
        self.__fetch(BootMsg.TypeEnum.ACK)
        
        logging.info('Bootloading completed successfully')
        return True

    def __do_get_appinfo(self, appname):
        raise NotImplementedError()
    
    def __do_set_parameter(self, appname, offset, chunk):
        raise NotImplementedError()
        
    def __do_get_parameter(self, appname, offset, chunk):
        raise NotImplementedError()
    
#==============================================================================

### DEPRECATED
class SimpleBootloader:
    def __init__(self, transport, boot_topic):
        self.__transport = transport
        self.__boot_topic = boot_topic
        
    def __abort(self):
        try:
            nack = BootMsg()
            nack.set_nack(0)
            data = nack.marshal()
            self.__transport.send_message(self.__boot_topic, data)
            self.__transport.send_message(self.__boot_topic, data)
            self.__transport.send_message(self.__boot_topic, data)
        except:
            pass
        
    def run(self, appname, hexpath, stacklen, ldexe, ldscript, ldoself, ldmap, ldobjelf, ldobjs):
        self.__transport.open()
        try:
            self.__do_run(appname, hexpath, stacklen, ldexe, ldscript, ldoself, ldmap, ldobjelf, ldobjs)
        except KeyboardInterrupt:
            logging.warning('Bootloading aborted by user')
            self.__abort()
            self.__transport.close()
        except:
            logging.warning('Unhandled exception occurred while bootloading')
            self.__abort()
            self.__transport.close()
            raise
        
    def __do_run(self, appname, hexpath, stacklen, ldexe, ldscript, ldoself, ldmap, ldobjelf, ldobjs):
        
        def send(msg):
            self.__transport.send_message(self.__boot_topic, msg.marshal())
        
        def recv(ignore_heartbeat=True):
            while True:
                fields = self.__transport.recv()
                if fields[0] == Transport.TypeEnum.MESSAGE and fields[1] == self.__boot_topic:
                    msg = BootMsg()
                    msg.unmarshal(fields[2])
                    logging.debug('received BootMsg_%s' % repr(msg))
                    if msg.type == BootMsg.TypeEnum.HEARTBEAT and ignore_heartbeat:
                        continue
                    return msg
        
        # Stop all middlewares
        logging.info('Stopping all middlewares')
        self.__transport.send_stop()
        logging.info('Awaiting heartbeat from target bootloader')
        while True:
            msg = recv(False)
            if msg.type == BootMsg.TypeEnum.HEARTBEAT:
                break
            
        
        # Get the length of each section
        logging.info('Reading section lengths from "%s":' % ldobjelf)
        pgmlen, bsslen, datalen = 0, 0, 0
        with open(ldobjelf, 'rb') as f:
            elffile = ELFFile(f)
            for section in elffile.iter_sections():
                if   section.name == '.text':   pgmlen  = int(section['sh_size'])
                elif section.name == '.bss':    bsslen  = int(section['sh_size'])
                elif section.name == '.data':   datalen = int(section['sh_size'])
        
        logging.info('  .text = 0x%0.8X (%d)' % (pgmlen, pgmlen))
        logging.info('  .bss  = 0x%0.8X (%d)' % (bsslen, bsslen))
        logging.info('  .data = 0x%0.8X (%d)' % (datalen, datalen))
        
        # Send the section sizes and app name
        logging.info('Sending section lengths to target module; in addition:')
        logging.info('  stacklen = 0x%0.8X (%d)' % (stacklen, stacklen))
        logging.info('  appname  = "%s"' % appname)
        
        setup_request = BootMsg()
        setup_request.set_request(pgmlen, bsslen, datalen, stacklen, appname)
        send(setup_request)
        
        # Receive the allocated addresses
        logging.info('Receiving the allocated addresses from target module:')
        setup_response = recv()
        pgmadr     = setup_response.setup_response.pgmadr
        bssadr     = setup_response.setup_response.bssadr
        dataadr    = setup_response.setup_response.dataadr
        datapgmadr = setup_response.setup_response.datapgmadr
        
        logging.info('  .text = 0x%0.8X' % pgmadr)
        logging.info('  .bss  = 0x%0.8X' % bssadr)
        logging.info('  .data = 0x%0.8X' % dataadr)
        logging.info('  const = 0x%0.8X' % datapgmadr)
        
        # Link to the OS symbols
        args = [
            ldexe,
            '--script', ldscript,
            '--just-symbols', ldoself,
            '-Map', ldmap,
            '-o', ldobjelf,
            '--section-start', '.text=0x%0.8X' % pgmadr,
            '--section-start', '.bss=0x%0.8X' % bssadr,
            '--section-start', '.data=0x%0.8X' % dataadr
        ] + list(ldobjs)
        logging.info('Linker command line:')
        logging.info('  ' + ' '.join([ arg for arg in args ]))
        
        logging.info('Linking object files')
        if 0 != subprocess.call(args):
            raise RuntimeError("Cannot link '%s' with '%s'" % (ldoself, ldobjelf))
        logging.info('Generating executable')
        if 0 != subprocess.call(['make']):
            raise RuntimeError("Cannot make the application binary")
        
        def find_address(elffile, name):
            dwarfinfo = elffile.get_dwarf_info()
            for CU in dwarfinfo.iter_CUs():
                for DIE in CU.iter_DIEs():
                    try:
                        if DIE.tag == 'DW_TAG_subprogram' and DIE.attributes['DW_AT_name'].value == name:
                            return int(DIE.attributes['DW_AT_low_pc'].value) + self.THREAD_PC_OFFSET
                    except KeyError: continue
            raise RuntimeError('Symbol "%s" not found' % name)
        
        logging.info('Reading final ELF file "%s"' % ldobjelf)
        with open(ldobjelf, 'rb') as f:
            elffile = ELFFile(f)
            threadadr = find_address(elffile, self.APP_THREAD_SYMBOL)
        logging.info('App entry point at 0x%0.8X' % threadadr)
        
        # Read the generated IHEX file and remove unused records
        logging.info('Reading IHEX file')
        with open(hexpath, 'r') as f:
            hexdata = f.readlines()
        
        logging.info('Removing unused records:')
        ihex_records = []
        for line in hexdata:
            line = line.strip()
            if len(line) == 0: continue
            record = IhexRecord()
            record.parse_ihex(line)
            if record.type == IhexRecord.TypeEnum.DATA:
                # Split into smaller packets
                while record.count > IhexRecord.MAX_DATA_LENGTH:
                    initial = IhexRecord()
                    initial.count = IhexRecord.MAX_DATA_LENGTH
                    initial.offset = record.offset
                    initial.type = IhexRecord.TypeEnum.DATA
                    initial.data = record.data[:IhexRecord.MAX_DATA_LENGTH]
                    ihex_records.append(initial)
                    
                    record.count -= IhexRecord.MAX_DATA_LENGTH
                    record.offset += IhexRecord.MAX_DATA_LENGTH
                    record.data = record.data[IhexRecord.MAX_DATA_LENGTH:]
                
                ihex_records.append(record)
                
            elif record.type != IhexRecord.TypeEnum.START_SEGMENT_ADDRESS and \
                 record.type != IhexRecord.TypeEnum.START_LINEAR_ADDRESS:
                ihex_records.append(record)
            else:
                logging.info('  ' + str(record))
        
        # Insert the thread entry point record
        record = IhexRecord()
        record.count = 4
        record.offset = 0
        record.type = IhexRecord.TypeEnum.START_LINEAR_ADDRESS
        record.data = struct.pack('>L', threadadr)
        record.checksum = record.compute_checksum()
        record.check_valid()
        logging.info('Adding app entry point record:')
        logging.info('  ' + str(record))
        ihex_records.insert(0, record)
        
        # Send IHEX records
        logging.info('Sending IHEX records to target module:')
        ihexmsg = BootMsg()
        ihexmsg.type = BootMsg.TypeEnum.IHEX_RECORD
        for record in ihex_records:
            ihexmsg.ihex_record = record
            logging.info('  ' + str(record))
            send(ihexmsg)
            response = recv()
            if response.type != BootMsg.TypeEnum.ACK:
                raise RuntimeError("Expected ACK, received '%s'" % response)
        
        logging.info('Bootloading completed successfully')

#==============================================================================

