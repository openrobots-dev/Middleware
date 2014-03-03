#pragma once

#include "time.hpp"

namespace r2p {

class PWMMsg: public Message {
public:
	uint16_t pwm;
}R2P_PACKED;

class PWM2Msg: public Message {
public:
	int16_t pwm1;
	int16_t pwm2;
}R2P_PACKED;

class PWM3Msg: public Message {
public:
	int16_t pwm1;
	int16_t pwm2;
	int16_t pwm3;
}R2P_PACKED;

class SpeedMsg: public Message {
public:
	uint16_t speed;
}R2P_PACKED;

class Speed2Msg: public Message {
public:
	float value[2];
}R2P_PACKED;

class Speed3Msg: public Message {
public:
	float value[3];
}R2P_PACKED;

class QEIMsg: public Message {
public:
	int16_t delta;
}R2P_PACKED;

class tQEIMsg: public Message {
public:
	timestamp_t timestamp;
	int16_t delta;
}R2P_PACKED;

class EncoderMsg: public Message {
public:
	float delta;
}R2P_PACKED;

class Encoder2Msg: public Message {
public:
	float left_delta;
	float right_delta;
}R2P_PACKED;

class tEncoderMsg: public Message {
public:
	timestamp_t timestamp;
	float delta;
}R2P_PACKED;

class Velocity3Msg: public Message {
public:
	float x;
	float y;
	float w;
}R2P_PACKED;

} /* namespace r2p */

#define PWM_ID				(0x20 << 8)
#define PWM2_ID				(0x21 << 8)
#define PWM3_ID				(0x22 << 8)
#define QEI_ID				(0x23 << 8)
#define QEI1_ID				(0x24 << 8)
#define QEI2_ID				(0x25 << 8)
#define SPEED2_ID			(0x26 << 8)
#define SPEED3_ID			(0x27 << 8)
#define ENCODER1_ID			(0x28 << 8)
#define ENCODER2_ID			(0x29 << 8)
#define ENCODER3_ID			(0x2A << 8)
#define VELOCITY_ID			(0x2B << 8)
#define VEL_CMD_ID			(0x2C << 8)
#define STEER_ENCODER_ID	(0x2D << 8)
#define WHEEL_ENCODERS_ID	(0x2E << 8)

