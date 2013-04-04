import pcan
from ctypes import *

can = pcan.CANIX()

errno = can.init()

if errno != 0:
	print "CAN bus error 0x%04x" % code

while (True):
	msg = pcan.Msg(0, 0, 0, (0,)*8)
	dword = can.read(pointer(msg))
	if dword != pcan.ERR_QRCVEMPTY:
		print msg.ID
		print msg.LEN
		print "{} {} {} {} {} {} {} {}".format(msg.DATA[0], msg.DATA[1], msg.DATA[2], msg.DATA[3], msg.DATA[4], msg.DATA[5], msg.DATA[6], msg.DATA[7])
		print can.status()
		print ""
