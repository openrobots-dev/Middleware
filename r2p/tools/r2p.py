
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
            return ''.join([ self.read_char() for i in range(length) ])
    
    
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
        
        # Receive the incoming message
        parser.expect_char('@')
        deadline = parser.read_unsigned(4)
        parser.expect_char(':')
        length = parser.read_unsigned(1)
        topic = parser.read_string(length)
        parser.expect_char(':')
        
        if length > 0: # Normal message
            length = parser.read_unsigned(1)
            payload = parser.read_bytes(length)
            parser.check_eol()
            return (Transport.TypeEnum.MESSAGE, topic, payload)
        
        else: # Management message
            typechar = parser.read_char()
            
            if typechar == 'p' or typechar == 'P':
                parser.expect_char(':')
                length = parser.read_unsigned(1)
                module = parser.read_string(length)
                parser.expect_char(':')
                length = parser.read_unsigned(1)
                topic = parser.read_string(length)
                parser.check_eol()
                return (Transport.TypeEnum.ADVERTISEMENT, topic)
            
            elif typechar == 's' or typechar == 'S':
                queue_length = parser.read_unsigned(1)
                parser.expect_char(':')
                length = parser.read_unsigned(1)
                module = parser.read_string(length)
                parser.expect_char(':')
                length = parser.read_unsigned(1)
                topic = parser.read_string(length)
                parser.check_eol()
                return (Transport.TypeEnum.SUBSCRIPTION, topic, queue_length)
            
            elif typechar == 't' or typechar == 'T':
                parser.check_eol()
                return (Transport.TypeEnum.STOP,)
            
            elif typechar == 'r' or typechar == 'R':
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
        return ':%0.2X%0.4X%0.2X%s%0.2X' % (self.count, self.offset, self.type,
                                            _str2hexb(self.data), self.checksum)
    
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

    def compute_checksum(self):
        cs = (_uintsum(self.count) + _uintsum(self.offset) + _uintsum(self.type)) & 0xFF
        for b in self.data:
            cs = (cs + ord(b)) & 0xFF
        return 0x100 - cs
    
    def marshal(self):
        return struct.pack('<BHB%dsB' % self.MAX_DATA_LENGTH,
                           self.count, self.offset, self.type, self.data, self.checksum)
    
    def unmarshal(self, data):
        self.count, self.offset, self.type, self.data, self.checksum = \
            struct.unpack('<BHB%dsB' % self.MAX_DATA_LENGTH, data)
        self.data = self.data[:self.count]
    
    def parse_ihex(self, entry):
        if entry[0] != ':': raise ValueError("Entry '%s' does not start with ':'" % entry)
        self.count = int(entry[1:3], 16)
        explen = 1 + 2 * (1 + 2 + 1 + self.count + 1)
        if len(entry) < explen:
            raise ValueError("len('%s') < %d" % (entry, explen))
        self.offset = int(entry[5:7] + entry[3:5], 16)
        self.type = int(entry[7:9], 16)
        entry = entry[9:]
        self.data = str(bytearray([ int(entry[i : (i + 2)], 16) for i in range(0, 2 * self.count, 2) ]))
        self.checksum = int(entry[(2 * self.count) : (2 * self.count + 2)], 16)
        self.check_valid()
        return self

#==============================================================================

class BootloaderMsg(Serializable):
    _LENGTH = 27
    
    class TypeEnum:
        NACK = 0
        ACK = 1
        SETUP_REQUEST = 2
        SETUP_RESPONSE = 3
        IHEX_RECORD = 4
        
        
    class SetupRequest(Serializable):
        def __init__(self):
            self.pgmlen = 0
            self.bsslen = 0
            self.datalen = 0
            self.stacklen = 0
            self.appname = ''
            self.checksum = 0
        
        def marshal(self):
            return struct.pack('<LLLL%dsB' % NODE_NAME_MAX_LENGTH,
                self.pgmlen, self.bsslen, self.datalen, self.stacklen, self.appname, self.checksum)
        
        def unmarshal(self, data):
            self.pgmlen, self.bsslen, self.datalen, self.stacklen, self.appname, self.checksum = \
                struct.unpack('<LLLL%dsB' % NODE_NAME_MAX_LENGTH, data)

        def compute_checksum(self):
            cs = (_uintsum(self.pgmlen) + _uintsum(self.bsslen) + \
                  _uintsum(self.datalen) + _uintsum(self.stacklen)) & 0xFF
            for c in self.appname:
                cs = (cs + ord(c)) & 0xFF
            return 0x100 - cs


    class SetupResponse(Serializable):
        def __init__(self):
            self.pgmadr = 0
            self.bssadr = 0
            self.dataadr = 0
            self.datapgmadr = 0
            self.checksum = 0
        
        def marshal(self):
            return struct.pack('<LLLLB', self.pgmadr, self.bssadr, self.dataadr,
                               self.datapgmadr, self.checksum)
            
        def unmarshal(self, data):
            self.pgmadr, self.bssadr, self.dataadr, self.datapgmadr,
            self.checksum = struct.unpack('<LLLLB', data)
            
        def compute_checksum(self):
            return 0x100 - ((_uintsum(self.pgmadr) + _uintsum(self.bssadr) + \
                             _uintsum(self.dataadr) + _uintsum(self.datapgmadr)) & 0xFF)


    def __init__(self):
        self.setup_request = self.SetupRequest()
        self.setup_response = self.SetupResponse()
        self.ihex_record = IhexRecord()
        self.type = -1
        
    def __repr__(self):
        return _str2hexb(self.marshal())
        
    def set_nack(self):
        self.type = self.TypeEnum.NACK
        
    def set_ack(self):
        self.type = self.TypeEnum.ACK
        
    def set_request(self, pgmlen, bsslen, datalen, stacklen, appname):
        self.type = self.TypeEnum.SETUP_REQUEST
        self.setup_request.pgmlen = int(pgmlen) 
        self.setup_request.bsslen = int(bsslen) 
        self.setup_request.datalen = int(datalen)
        self.setup_request.stacklen = int(stacklen)
        self.setup_request.appname = str(appname)
        self.setup_request.checksum = self.setup_request.compute_checksum()
        
    def set_response(self, pgmadr, bssadr, dataadr, datapgmadr):
        self.type = self.TypeEnum.SETUP_RESPONSE
        self.pgmadr = int(pgmadr)
        self.bssadr = int(bssadr)
        self.dataadr = int(dataadr)
        self.datapgmadr = int(datapgmadr)
        self.setup_response.checksum = self.setup_response.compute_checksum()
        
    def set_ihex(self, ihex_entry_line):
        self.type = self.TypeEnum.IHEX_RECORD
        self.ihex_record.parse_ihex(ihex_entry_line)
    
    def marshal(self):
        if   self.type == self.TypeEnum.NACK:           bytes = ''
        elif self.type == self.TypeEnum.ACK:            bytes = ''
        elif self.type == self.TypeEnum.SETUP_REQUEST:  bytes = self.setup_request.marshal()
        elif self.type == self.TypeEnum.SETUP_RESPONSE: bytes = self.setup_response.marshal()
        elif self.type == self.TypeEnum.IHEX_RECORD:    bytes = self.ihex_record.marshal()
        else: raise ValueError('Unknown bootloader message subtype %d' % self.type)
        return struct.pack('<%dsB' % (self._LENGTH - 1), bytes, self.type)
    
    def unmarshal(self, data):
        if len(data) < self._LENGTH:
            raise ValueError("len('%s') < %d", data, len(data))
        _, self.type = struct.unpack('<%dsB' % (self._LENGTH - 1), data)
        
        if   self.type == self.TypeEnum.NACK:           pass
        elif self.type == self.TypeEnum.ACK:            pass
        elif self.type == self.TypeEnum.SETUP_REQUEST:  self.setup_request.unmarshal(data)
        elif self.type == self.TypeEnum.SETUP_RESPONSE: self.setup_response.unmarshal(data)
        elif self.type == self.TypeEnum.IHEX_RECORD:    self.ihex_record.unmarshal(data)
        else: raise ValueError('Unknown bootloader message subtype %d' % self.type)

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
            self.__transport.close()
            raise
        
    def __do_run(self, appname, hexpath, stacklen, ldexe, ldscript, ldoself, ldmap, ldobjelf, ldobjs):
        
        def send(msg):
            self.__transport.send_message(self.__boot_topic, msg.marshal())
        
        def recv():
            while True:
                fields = self.__transport.recv()
                if fields[0] == Transport.TypeEnum.MESSAGE and fields[1] == self.__boot_topic:
                    msg = BootloaderMsg()
                    msg.unmarshal(fields[2])
                    return msg
        
        # Stop all middlewares
        logging.info('Stopping all middlewares')
        self.__transport.send_stop()
        logging.debug('Signal sent, sleeping for 3 seconds...')
        time.sleep(3)
        logging.debug('... awake again')
        
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
        sectopt = [ '--section-start', '.text=0x%0.8X' % pgmadr,
                    '--section-start', '.bss=0x%0.8X' % bssadr,
                    '--section-start', '.data=0x%0.8X' % dataadr ]
        args = [ ldexe, '--script', ldscript, '--just-symbols', ldoself, '-Map', ldmap, '-o', ldobjelf ] + sectopt + ldobjs
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
        record.checksum = record.compute_checksum()
        record.check_valid()
        logging.info('Adding app entry point record:')
        logging.info('  ' + str(record))
        ihex_records.append(record)
            
        response = recv()
        if response.type != BootloaderMsg.TypeEnum.ACK:
            raise RuntimeError("Expected ACK, received '%s'" % response)
        
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

