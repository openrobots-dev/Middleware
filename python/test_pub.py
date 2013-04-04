from ctypes import *
from time import sleep

import pcan

can = pcan.CANIX()

errno = can.init()

if errno != 0:
	print "CAN bus error 0x%04x" % code

while (True):
	msg = pcan.Msg(478166400, 0x02, 6, (14, 32, 1, 0, 0, 0, 0, 0))
	dword = can.write(pointer(msg))
	print dword
	print "%x" % can.status()
	sleep(1)

	msg = pcan.Msg(478166400, 0x02, 6, (14, 32, 0, 0, 0, 0, 0, 0))
	dword = can.write(pointer(msg))
	print dword
	print "%x" % can.status()
	sleep(1)
