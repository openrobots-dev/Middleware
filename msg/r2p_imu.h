#ifndef __R2P_IMU_H_
#define __R2P_IMU_H_

#define IMURAW9_ID		(41 << 8)
#define IMUATT_ID		(42 << 8)
#define IMUTILT_ID		(43 << 8)

#include "BaseMessage.hpp"

#include "r2p_time.h"

struct IMURaw9: public BaseMessage {
	int16_t acc_x;
	int16_t acc_y;
	int16_t acc_z;
	int16_t gyro_x;
	int16_t gyro_y;
	int16_t gyro_z;
	int16_t mag_x;
	int16_t mag_y;
	int16_t mag_z;
}__attribute__((packed));

struct tIMURaw9: public BaseMessage {
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
}__attribute__((packed));

struct IMUAttitude: public BaseMessage {
	float roll;
	float pitch;
	float yaw;
}__attribute__((packed));

struct IMUTilt: public BaseMessage {
	float angle;
	float rate;
}__attribute__((packed));

#endif /* __R2P_IMU_H_ */
