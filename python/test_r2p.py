from ctypes import *
from time import sleep
from r2p import *

class LEDData(BaseMessage):
  _fields_ = [
    ('pin', c_uint16), # pin number
    ('set', c_int32), # on/off
  ]


r2p = R2P()

status = 1

while (True):
	msg = LEDData(13, 0)
	dword = r2p.publish("LED23", msg)
	print "."
	msg = LEDData(14, 1)
	dword = r2p.publish("LED23", msg)
	print "."
	sleep(0.5)
	msg = LEDData(13, 1)
	dword = r2p.publish("LED23", msg)
	print "."
	msg = LEDData(14, 0)
	dword = r2p.publish("LED23", msg)
	print "."
	sleep(0.5)
