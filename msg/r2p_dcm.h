#ifndef __R2P_DCM_H__
#define __R2P_DCM_H__

#define PWM123_ID		(21 << 8)
#define PWM_ID			(22 << 8)
#define QEI_ID			(23 << 8)
#define SPEED123_ID		(24 << 8)
#define PIDSETUP_ID		(25 << 8)
#define PWM12_ID		(26 << 8)
#define SPEED12_ID		(27 << 8)

#include "BaseMessage.hpp"

#include "r2p_time.h"

class PWM: public BaseMessage {
public:
	int16_t pwm;
}__attribute__((packed));

class PWM2: public BaseMessage {
public:
	int16_t pwm1;
	int16_t pwm2;
}__attribute__((packed));

class PWM3: public BaseMessage {
public:
	int16_t pwm1;
	int16_t pwm2;
	int16_t pwm3;
}__attribute__((packed));

class QEI: public BaseMessage {
public:
	int16_t delta;
}__attribute__((packed));

class tQEI: public BaseMessage {
public:
	timestamp_t timestamp;
	int16_t delta;
}__attribute__((packed));

class SpeedSetpoint: public BaseMessage {
public:
	int16_t speed;
}__attribute__((packed));

class SpeedSetpoint2: public BaseMessage {
public:
	int16_t speed1;
	int16_t speed2;
}__attribute__((packed));

class SpeedSetpoint3: public BaseMessage {
public:
	int16_t speed1;
	int16_t speed2;
	int16_t speed3;
}__attribute__((packed));

class PIDSetup: public BaseMessage {
public:
	int16_t kp;
	int16_t ki;
	int16_t kd;
}__attribute__((packed));

#endif /* __R2P_DCM_H__ */
