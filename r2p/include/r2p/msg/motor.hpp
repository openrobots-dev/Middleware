#pragma once

namespace r2p {

struct PWMMsg: public Message {
	uint16_t pwm;
}R2P_PACKED;

struct PWM2Msg: public Message {
	int16_t pwm1;
	int16_t pwm2;
}R2P_PACKED;

struct PWM3Msg: public Message {
	int16_t pwm1;
	int16_t pwm2;
	int16_t pwm3;
}R2P_PACKED;

struct QEIMsg: public Message {
	uint16_t delta;
}R2P_PACKED;

} /* namespace r2p */

#define PWM3_ID			(21 << 8)
#define PWM_ID			(22 << 8)
#define QEI_ID			(23 << 8)
#define SPEED3_ID		(24 << 8)
#define PIDSETUP_ID		(25 << 8)
#define PWM2_ID			(26 << 8)
#define SPEED2_ID		(27 << 8)
#define VELOCITY_ID     (28 << 8)

