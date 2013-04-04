from pcan import *
from pcan_abstract import *
import ctypes


class CANIOError(IOError):
  pass

class CANIX(AbstractCANIX):
      def __init__(self):
            O_RDWR = 2
            device_path = "/dev/pcanusb0"
            self.h = LICAN.open(device_path,O_RDWR)
            if self.h == 0:
                  raise CANIOError(
                    "Failed to open device: %s. Can handle is zero"%device_path)
            
      def init(self,baudrate_code = 0x14, msgtype = MSGTYPE_EXTENDED, dev = "/dev/pcanusb0"):
            return CAN.init(self.h,baudrate_code, msgtype)

      def resetfilter(self):
            return CAN.resetfilter(self.h)
      
      def msgfilter(self, from_id, to_id, can_msg_type):
            return CAN.msgfilter(self.h, from_id, to_id, can_msg_type)

      def close(self):
            return CAN.close(self.h)
            
      def status(self):
            return CAN.status(self.h);
      
      def write(self, msg_pointer):
            return CAN.write(self.h, msg_pointer)            

      def read(self, msg_pointer):
            outrdmsg = RdMsg(msg_pointer.contents,0,0)
            errno = LICAN.read_timeout(self.h, pointer(outrdmsg), 0) #poll (no timeout)
            if not errno:
                  ctypes.memmove( ctypes.byref(msg_pointer.contents), 
                        ctypes.byref(outrdmsg.Msg), 
                        ctypes.sizeof(msg_pointer.contents))
            return errno

# Copy all docstrings from our abstract superclass
for mName,mBody in CANIX.__dict__.iteritems():
      if callable(mBody) and hasattr(AbstractCANIX,mName):
            mBody.__doc__ = getattr(AbstractCANIX,mName).__doc__
