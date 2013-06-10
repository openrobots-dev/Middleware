import sys
from ctypes import *
import pcan

if len(sys.argv) < 2:
        print "Usage: " + sys.argv[0] + " ID"
        print "Set ID to 0 to sniff all packets"
        sys.exit()

filter_id = int(sys.argv[1])

can = pcan.CANIX()

errno = can.init()

if errno != 0:
	print "CAN bus error 0x%04x" % code

while (True):
	msg = pcan.Msg(0, 0, 0, (0,)*8)
	dword = can.read(pointer(msg))
	if dword != pcan.ERR_QRCVEMPTY:
		if (filter_id >= 0) and (filter_id != (msg.ID >> 15) & 0xFF):
			continue
		
		print "ID: {}.{}".format(int(msg.ID >> 15) & 0xFF, int(msg.ID >> 7) & 0xFF)
		print "LAXITY: {}".format(int(msg.ID >> 22) & 0x3F)
		print "FRAGMENT: {}".format(int(msg.ID) & 0x3F)
		print "PAYLOAD: {}".format(msg.LEN)
		print msg.LEN
		print "{} {} {} {} {} {} {} {}".format(msg.DATA[0], msg.DATA[1], msg.DATA[2], msg.DATA[3], msg.DATA[4], msg.DATA[5], msg.DATA[6], msg.DATA[7])
		print can.status()
		print ""
