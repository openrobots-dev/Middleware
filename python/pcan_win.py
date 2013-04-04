from pcan import *
from pcan_abstract import *

class CANIX(object):
	#TODO: turn baudrate codes into number and lookup (btr0btr1)
	def init(self, baudrate_code = 0x11c, msgtype = MSGTYPE_STANDARD, dev = None):
		errno = CAN.init(baudrate_code, msgtype)
		return errno
		#TODO: raise some error if errno is something other than 0
		
	def resetfilter(self):
		return CAN.resetfilter()
		
	def msgfilter(self, from_id = 0x000, to_id = 0x7ff, can_msg_type = MSGTYPE_STANDARD):
		return CAN.msgfilter(from_id,to_id,can_msg_type)
		
	def close(self):
		return CAN.close()
		
	def status(self):
		return CAN.status()
		
	def write(self, msg_pointer):
		return CAN.write(msg_pointer)
	
	def read(self, msg_pointer):
		return CAN.read(msg_pointer)
		
# Copy all docstrings from our abstract superclass
for mName,mBody in CANIX.__dict__.iteritems():
      if callable(mBody) and hasattr(AbstractCANIX,mName):
            mBody.__doc__ = getattr(AbstractCANIX,mName).__doc__	
