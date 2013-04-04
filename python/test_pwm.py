from ctypes import *
from time import sleep
from r2p import *

class PWMData(BaseMessage):
  _fields_ = [
    ('pwm', c_int16), # pwm value
  ]


r2p = R2P()

pwm = 0

while (True):
	msg = PWMData(pwm)
	dword = r2p.publish("PWM1", msg)
	print pwm
	pwm = pwm + 100
	if (pwm > 1000):
		pwm = 0
	sleep(0.1)
