from ctypes import *
from pcan import *
from r2p_topics import TOPICS

class BaseMessage(Structure):
  _pack_ = True


class R2P(object):
	def __init__(self):
		self.can = CANIX()
		errno = self.can.init()
		if errno != 0:
			print "CAN bus error 0x%04x" % code
			
		self.topics = TOPICS
		self.callbacks = dict()
	
	def map(self, topic, rtcan_id):
		self.topics[topic] = rtcan_id
		
	def publish(self, topic, message):
		can_frame = Msg((self.topics[topic] << 7), MSGTYPE_EXTENDED, sizeof(message), cast(pointer(message), POINTER(c_ubyte * 8)).contents)
		dword = self.can.write(pointer(can_frame))
		return dword

	def subscribe(self, topic, callback = None):
		self.callbacks[topic] = callback

	def spin(self):
		msg = Msg(0, 0, 0, (0,)*8)
		dword = self.can.read(pointer(msg))
		if dword == 0:
			rtcan_id = (msg.ID >> 7) & 0x7FFF
			topic = [item[0] for item in self.topics.items() if item[1] == rtcan_id]
			
			if len(topic) == 1:
				topic = topic[0]
				if topic in self.callbacks:
					callback = self.callbacks[topic]
					callback(msg)
