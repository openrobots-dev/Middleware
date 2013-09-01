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
            raise ValueError('Checksum is 0x%0.2X, expected 0x%0.2X',
                             checksum, self.compute_checksum())
    
#==============================================================================

class Serializable:
    
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
    
    def send_reset(self):
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
        line = '@%.8X:%.2X%s:%.2X%s' % (int(time.time()) & 0xFFFFFFFF,
                                        len(topic) & 0xFF, topic,
                                        len(payload) & 0xFF, _str2hexb(payload))
        self.__lineio.writeln(line)
    
    def send_advertisement(self, topic):
        assert 0 < len(topic) < 256
        assert 0 < len(_MODULE_NAME) <= 7
        line = '@%.8X:00:p:%.2X%s:%.2X%s' % (int(time.time()) & 0xFFFFFFFF,
                                             len(_MODULE_NAME), _MODULE_NAME,
                                             len(topic) & 0xFF, topic)
        self.__lineio.writeln(line)
    
    def send_subscription(self, topic, queue_length):
        assert 0 < len(topic) < 256
        assert 0 < queue_length < 256
        assert 0 < len(_MODULE_NAME) <= 7
        line = '@%.8X:00:s%.2X:%.2X%s:%.2X%s' % (int(time.time()) & 0xFFFFFFFF,
                                                 queue_length,
                                                 len(_MODULE_NAME), _MODULE_NAME,
                                                 len(topic) & 0xFF, topic)
        self.__lineio.writeln(line)
    
    def send_stop(self):
        line = '@%.8X:00:t' % (int(time.time()) & 0xFFFFFFFF)
        self.__lineio.writeln(line)
    
    def send_reset(self):
        line = '@%.8X:00:r' % (int(time.time()) & 0xFFFFFFFF)
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
        self.compute_checksum = 0
        
    def __repr__(self):
        return str(self.__dict__)
        
    def __str__(self):
        return ':%0.2X%0.4X%0.2X%s%0.2X' % (self.count, self.offset, self.type,
                                            _str2hexb(self.data), self.compute_checksum)
    
    def check_valid(self):
        if not (0 <= self.count <= 255):
            raise ValueError('not(0 <= count=%d <= 255)' % self.count)
        if self.count != len(self.data):
            raise ValueError('count=%d != len(data)=%d' % (self.count, len(self.data)))
        if not (0 <= self.offset <= 0xFFFF):
            raise ValueError('not(0 <= offset=0x%0.8X <= 0xFFFF)' % self.offset)
        if self.compute_checksum != self.compute_checksum():
            raise ValueError('compute_checksum=%d != expected=%d', (self.compute_checksum, self.compute_checksum()));
        
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

    def compute_checksum(self):
        cs = (_uintsum(self.count) + _uintsum(self.offset) + _uintsum(self.type)) & 0xFF
        for c in self.data:
            cs = (cs + ord(c)) & 0xFF
        return (0x100 - cs) & 0xFF
    
    def marshal(self):
        return struct.pack('<BHB%dsB' % self.MAX_DATA_LENGTH,
                           self.count, self.offset, self.type, self.data, self.compute_checksum)
    
    def unmarshal(self, data):
        self.count, self.offset, self.type, self.data, self.compute_checksum = \
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
        self.compute_checksum = int(entry[(2 * self.count) : (2 * self.count + 2)], 16)
        self.check_valid()
        return self

#==============================================================================

class BootloaderMsg(Serializable):
    MAX_PAYLOAD_LENGTH = 26
    
    class TypeEnum:
        NACK            = 0
        ACK             = 1
        HEARTBEAT       = 2
        SETUP_REQUEST   = 3
        SETUP_RESPONSE  = 4
        IHEX_RECORD     = 5
        
    
    class AckNack(Serializable):
        def __init__(self):
            self.to_seqn = -1
        
        def __repr__(self):
            return str(self.__dict__)
        
        def marshal(self):
            return struct.pack('<H', self.to_seqn)
        
        def unmarshal(self, data):
            self.to_seqn, = struct.unpack_from('<H', data)
        
        def compute_checksum(self):
            return _uintsum(self.to_seqn)
        
        
    class SetupRequest(Serializable):
        def __init__(self):
            self.pgmlen = 0
            self.bsslen = 0
            self.datalen = 0
            self.stacklen = 0
            self.appname = ''
            self.compute_checksum = 0
            
        def __repr__(self):
            return str(self.__dict__)
        
        def marshal(self):
            return struct.pack('<LLLL%dsB' % NODE_NAME_MAX_LENGTH,
                self.pgmlen, self.bsslen, self.datalen, self.stacklen, self.appname, self.compute_checksum)
        
        def unmarshal(self, data):
            self.pgmlen, self.bsslen, self.datalen, self.stacklen, self.appname, self.compute_checksum = \
                struct.unpack_from('<LLLL%dsB' % NODE_NAME_MAX_LENGTH, data)

        def compute_checksum(self):
            cs = (_uintsum(self.pgmlen) + _uintsum(self.bsslen) + \
                  _uintsum(self.datalen) + _uintsum(self.stacklen)) & 0xFF
            for c in self.appname:
                cs = (cs + ord(c)) & 0xFF
            return (0x100 - cs) & 0xFF


    class SetupResponse(Serializable):
        def __init__(self):
            self.pgmadr = 0
            self.bssadr = 0
            self.dataadr = 0
            self.datapgmadr = 0
            self.compute_checksum = 0
            
        def __repr__(self):
            return str(self.__dict__)
        
        def marshal(self):
            return struct.pack('<LLLLB', self.pgmadr, self.bssadr, self.dataadr,
                               self.datapgmadr, self.compute_checksum)
            
        def unmarshal(self, data):
            self.pgmadr, self.bssadr, self.dataadr, self.datapgmadr, self.compute_checksum = \
                struct.unpack_from('<LLLLB', data)
            
        def compute_checksum(self):
            cs = (_uintsum(self.pgmadr) + _uintsum(self.bssadr) + \
                  _uintsum(self.dataadr) + _uintsum(self.datapgmadr)) & 0xFF
            return (0x100 - cs) & 0xFF


    def __init__(self):
        self.compute_checksum = 0
        self.type = -1
        self.seqn = 0
        
        self.acknack = BootloaderMsg.AckNack()
        self.setup_request = BootloaderMsg.SetupRequest()
        self.setup_response = BootloaderMsg.SetupResponse()
        self.ihex_record = IhexRecord()
    
    def __repr__(self):
        if   self.type == BootloaderMsg.TypeEnum.NACK:
            return 'NACK(%s)' % repr(self.acknack)
        elif self.type == BootloaderMsg.TypeEnum.ACK:
            return 'ACK(%s)' % repr(self.acknack)
        elif self.type == BootloaderMsg.TypeEnum.HEARTBEAT:
            return 'HEARTBEAT()'
        elif self.type == BootloaderMsg.TypeEnum.SETUP_REQUEST:
            return 'SETUP_REQUEST(%s)' % repr(self.setup_request)
        elif self.type == BootloaderMsg.TypeEnum.SETUP_RESPONSE:
            return 'SETUP_RESPONSE(%s)' % repr(self.setup_response)
        elif self.type == BootloaderMsg.TypeEnum.IHEX_RECORD:
            return 'IHEX_RECORD(%s)' % repr(self.ihex_record)
        else:
            raise ValueError('Unknown bootloader message subtype %d' % self.type)
        
    def __str__(self):
        return _str2hexb(self.marshal())
        
    def compute_checksum(self):
        cs = 0
        for c in self.marshal()[1:]:
            cs = (cs + ord(c)) & 0xFF
        return (0x100 - cs) & 0xFF
    
    def update_checksum(self):
        self.compute_checksum = self.compute_checksum()
    
    def set_nack(self, to_seqn):
        self.type = BootloaderMsg.TypeEnum.NACK
        self.acknack.to_seqn = int(to_seqn)
        self.update_checksum()
        
    def set_ack(self, to_seqn):
        self.type = BootloaderMsg.TypeEnum.ACK
        self.acknack.to_seqn = int(to_seqn)
        self.update_checksum()
        
    def set_request(self, pgmlen, bsslen, datalen, stacklen, appname):
        self.type = self.TypeEnum.SETUP_REQUEST
        self.setup_request.pgmlen = int(pgmlen) 
        self.setup_request.bsslen = int(bsslen) 
        self.setup_request.datalen = int(datalen)
        self.setup_request.stacklen = int(stacklen)
        self.setup_request.appname = str(appname)
        self.setup_request.compute_checksum = self.setup_request.compute_checksum()
        self.update_checksum()
        
    def set_response(self, pgmadr, bssadr, dataadr, datapgmadr):
        self.type = BootloaderMsg.TypeEnum.SETUP_RESPONSE
        self.pgmadr = int(pgmadr)
        self.bssadr = int(bssadr)
        self.dataadr = int(dataadr)
        self.datapgmadr = int(datapgmadr)
        self.setup_response.compute_checksum = self.setup_response.compute_checksum()
        self.update_checksum()
        
    def set_ihex(self, ihex_entry_line):
        self.type = BootloaderMsg.TypeEnum.IHEX_RECORD
        self.ihex_record.parse_ihex(ihex_entry_line)
        self.update_checksum()
    
    def marshal(self):
        if   self.type == BootloaderMsg.TypeEnum.NACK or \
             self.type == BootloaderMsg.TypeEnum.ACK:
            payload = self.acknack.marshal()
        elif self.type == BootloaderMsg.TypeEnum.HEARTBEAT:
            payload = ''
        elif self.type == BootloaderMsg.TypeEnum.SETUP_REQUEST:
            payload = self.setup_request.marshal()
        elif self.type == BootloaderMsg.TypeEnum.SETUP_RESPONSE:
            payload = self.setup_response.marshal()
        elif self.type == BootloaderMsg.TypeEnum.IHEX_RECORD:
            payload = self.ihex_record.marshal()
        else:
            raise ValueError('Unknown bootloader message subtype %d' % self.type)
        return struct.pack('<BBH%ds' % self.MAX_PAYLOAD_LENGTH,
                           self.compute_checksum, self.type, self.seqn, payload)
    
    def unmarshal(self, data):
        if len(data) < (1 + 1 + 2 + self.MAX_PAYLOAD_LENGTH):
            raise ValueError("len('%s') < %d", data, len(data))
        self.compute_checksum, self.type, self.seqn, payload = \
            struct.unpack_from('<BBH%ds' % self.MAX_PAYLOAD_LENGTH, data)
        
        if   self.type == BootloaderMsg.TypeEnum.NACK or \
             self.type == BootloaderMsg.TypeEnum.ACK:
            self.acknack.unmarshal(payload)
        elif self.type == BootloaderMsg.TypeEnum.HEARTBEAT:
            pass
        elif self.type == BootloaderMsg.TypeEnum.SETUP_REQUEST:
            self.setup_request.unmarshal(payload)
        elif self.type == BootloaderMsg.TypeEnum.SETUP_RESPONSE:
            self.setup_response.unmarshal(payload)
        elif self.type == BootloaderMsg.TypeEnum.IHEX_RECORD:
            self.ihex_record.unmarshal(payload)
        else:
            raise ValueError('Unknown bootloader message subtype %d' % self.type)

#==============================================================================

class SimpleBootloader:
    ENTRY_THREAD_NAME = 'app_thread'
    THREAD_PC_OFFSET = 1
    
    def __init__(self, transport, boot_topic):
        self.__transport = transport
        self.__boot_topic = boot_topic
        
        
    def run(self, appname, hexpath, stacklen, ldexe, ldscript, ldoself, ldmap, ldobjelf, ldobjs):
        self.__transport.open()
        try:
            self.__do_run(appname, hexpath, stacklen, ldexe, ldscript, ldoself, ldmap, ldobjelf, ldobjs)
        except:
            try:
                nack = BootloaderMsg()
                nack.set_nack(0)
                data = nack.marshal()
                self.__transport.send_message(self.__boot_topic, data)
                self.__transport.send_message(self.__boot_topic, data)
                self.__transport.send_message(self.__boot_topic, data)
            except:
                pass
            self.__transport.close()
            raise
        
    def __do_run(self, appname, hexpath, stacklen, ldexe, ldscript, ldoself, ldmap, ldobjelf, ldobjs):
        
        def send(msg):
            self.__transport.send_message(self.__boot_topic, msg.marshal())
        
        def recv(ignore_heartbeat=True):
            while True:
                fields = self.__transport.recv()
                if fields[0] == Transport.TypeEnum.MESSAGE and fields[1] == self.__boot_topic:
                    msg = BootloaderMsg()
                    msg.unmarshal(fields[2])
                    logging.debug('received BootloaderMsg_%s' % repr(msg))
                    if msg.type == BootloaderMsg.TypeEnum.HEARTBEAT and ignore_heartbeat:
                        continue
                    return msg
        
        # Stop all middlewares
        logging.info('Stopping all middlewares')
        self.__transport.send_stop()
        logging.info('Awaiting heartbeat from target bootloader')
        while True:
            msg = recv(False)
            if msg.type == BootloaderMsg.TypeEnum.HEARTBEAT:
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
        
        setup_request = BootloaderMsg()
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
            threadadr = find_address(elffile, self.ENTRY_THREAD_NAME)
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
        record.compute_checksum = record.compute_checksum()
        record.check_valid()
        logging.info('Adding app entry point record:')
        logging.info('  ' + str(record))
        ihex_records.insert(0, record)
        
        # Send IHEX records
        logging.info('Sending IHEX records to target module:')
        ihexmsg = BootloaderMsg()
        ihexmsg.type = BootloaderMsg.TypeEnum.IHEX_RECORD
        for record in ihex_records:
            ihexmsg.ihex_record = record
            logging.info('  ' + str(record))
            send(ihexmsg)
            response = recv()
            if response.type != BootloaderMsg.TypeEnum.ACK:
                raise RuntimeError("Expected ACK, received '%s'" % response)
        
        logging.info('Bootloading completed successfully')

#==============================================================================

