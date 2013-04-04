from ctypes import *
from time import sleep
from r2p import *

class LEDData(BaseMessage):
  _fields_ = [
    ('pin', c_uint16), # pin number
    ('set', c_int32), # on/off
  ]
  
def callback(m):
	msg = cast(pointer(m.DATA), POINTER(LEDData)).contents
	print "%d %d" % (msg.pin, msg.set)


r2p = R2P()

status = 1

r2p.subscribe("LED23", callback)

while (True):
	r2p.spin()
