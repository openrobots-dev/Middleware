from ctypes import *
from time import sleep
from r2p import *

class QEIData(BaseMessage):
  _fields_ = [
    ('value', c_uint16), # QEI absolute value
    ('timestamp', c_uint32), # timestamp in ms
  ]
  
def callback1(m):
	msg = cast(pointer(m.DATA), POINTER(QEIData)).contents
	print "1: %d %d" % (msg.timestamp, msg.value)

def callback2(m):
	msg = cast(pointer(m.DATA), POINTER(QEIData)).contents
	print "2: %d %d" % (msg.timestamp, msg.value)

def callback3(m):
	msg = cast(pointer(m.DATA), POINTER(QEIData)).contents
	print "3: %d %d" % (msg.timestamp, msg.value)

r2p = R2P()

status = 1

r2p.subscribe("QEI1", callback1)
r2p.subscribe("QEI2", callback2)
r2p.subscribe("QEI3", callback3)

while (True):
	r2p.spin()
