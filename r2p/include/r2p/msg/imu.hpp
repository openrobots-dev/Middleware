#pragma once

#include "time.hpp"

namespace r2p {

struct TiltMsg: public Message {
	float angle;
} R2P_PACKED;

struct IMURaw9: public Message {
	int16_t acc_x;
	int16_t acc_y;
	int16_t acc_z;
	int16_t gyro_x;
	int16_t gyro_y;
	int16_t gyro_z;
	int16_t mag_x;
	int16_t mag_y;
	int16_t mag_z;
} R2P_PACKED;

struct tIMURaw9: public Message {
	timestamp_t timestamp;
	int16_t acc_x;
	int16_t acc_y;
	int16_t acc_z;
	int16_t gyro_x;
	int16_t gyro_y;
	int16_t gyro_z;
	int16_t mag_x;
	int16_t mag_y;
	int16_t mag_z;
} R2P_PACKED;

#define IMURAW9_ID		(41 << 8)
#define IMUATT_ID		(42 << 8)
#define TILT_ID		    (43 << 8)

}
