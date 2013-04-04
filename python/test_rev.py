from ctypes import *
from time import sleep
from r2p import *

class PWM123Data(BaseMessage):
  _fields_ = [
    ('pwm1', c_int16), # pwm value for M1
    ('pwm2', c_int16), # pwm value for M2
    ('pwm3', c_int16), # pwm value for M3
  ]


r2p = R2P()

pwm = 0

while (True):
	msg = PWM123Data(-pwm, 0, -pwm)
	dword = r2p.publish("PWM123", msg)
	print pwm
	pwm = pwm + 100
	if (pwm > 2000):
		pwm = 0
	sleep(0.2)
