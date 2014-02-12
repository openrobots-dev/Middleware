#include "ch.h"
#include "hal.h"

#include "config.h"

#include <r2p/common.hpp>
#include <r2p/Middleware.hpp>
#include <r2p/Node.hpp>
#include <r2p/Publisher.hpp>
#include <r2p/Subscriber.hpp>

#include <r2p/node/pid.hpp>
#include <r2p/msg/motor.hpp>

namespace r2p {

extern PWMConfig pwmcfg;
extern QEIConfig qeicfg;

#define M1 72
#define M2 73
#define M3 89


/*===========================================================================*/
/* Robot parameters.                                                         */
/*===========================================================================*/
#define _L        0.30f    // Wheel distance [m]
#define _R        0.05f    // Wheel radius [m]
#define _TICKS 2000.0f
#define _RATIO 14.0f
#define _PI 3.14159265359f

#define R2T(r) ((r / (2 * _PI)) * (_TICKS * _RATIO))
#define T2R(t) ((t / (_TICKS * _RATIO)) * (2 * _PI))

#define M2T(m) (m * _TICKS * _RATIO)/(2 * _PI * _R)

/*===========================================================================*/
/* Utility functions.                                                        */
/*===========================================================================*/

uint8_t stm32_id8(void) {
	const unsigned long * uid = (const unsigned long *) 0x1FFFF7E8;

	return (uid[2] & 0xFF);
}

/*===========================================================================*/
/* Motor control nodes.                                                      */
/*===========================================================================*/

/*
 * PWM subscriber node
 */

bool callback(const PWM2Msg &msg) {
	int16_t pwm;

	switch (stm32_id8()) {
	case M1:
		pwm = msg.pwm1;
		break;
	case M2:
		pwm = msg.pwm2;
		break;
	default:
		pwm = 0;
		return true;
	}

	chSysLock()
	;

	if (pwm > 0) {
		pwm_lld_enable_channel(&PWM_DRIVER, 0, pwm);
		pwm_lld_enable_channel(&PWM_DRIVER, 1, 0);
	} else {
		pwm_lld_enable_channel(&PWM_DRIVER, 0, 0);
		pwm_lld_enable_channel(&PWM_DRIVER, 1, -pwm);
	}

	chSysUnlock();

	return true;
}

msg_t pwm2sub_node(void * arg) {
	Node node("pwm2sub");
	Subscriber<PWM2Msg, 5> sub(callback);

	(void) arg;

	chRegSetThreadName("pwm2sub");

	palSetPad(DRIVER_GPIO, DRIVER_RESET);
	chThdSleepMilliseconds(500);
	pwmStart(&PWM_DRIVER, &pwmcfg);

	node.subscribe(sub, "pwm2");

	for (;;) {
		if (!node.spin(Time::ms(100))) {
			// Stop motor if no messages for 100 ms
			chSysLock()
			;
			pwm_lld_enable_channel(&PWM_DRIVER, 0, 0);
			pwm_lld_enable_channel(&PWM_DRIVER, 0, 0);
			chSysUnlock();
		}
	}
	return CH_SUCCESS;
}

/*
 * PID node.
 */

PID speed_pid;
int index = 0;
int pwm = 0;

bool enc_callback(const r2p::tEncoderMsg &msg) {

	pwm = speed_pid.update(msg.delta);

	chSysLock();

	if (pwm > 0) {
		pwm_lld_enable_channel(&PWM_DRIVER, 1, pwm);
		pwm_lld_enable_channel(&PWM_DRIVER, 0, 0);
	} else {
		pwm_lld_enable_channel(&PWM_DRIVER, 1, 0);
		pwm_lld_enable_channel(&PWM_DRIVER, 0, -pwm);
	}
	chSysUnlock();

	palTogglePad(LED2_GPIO, LED2);

	return true;
}


msg_t pid2_node(void * arg) {
	Node node("pid");
	Subscriber<Speed2Msg, 5> speed_sub;
	Subscriber<tEncoderMsg, 5> enc_sub(enc_callback);
	Speed2Msg * msgp;
	Time last_setpoint(0);

	(void) arg;

	chRegSetThreadName("pid");

	speed_pid.config(450.0, 0.125, 0.0, 0.01, -4095.0, 4095.0);

	switch (stm32_id8()) {
	case M1:
		index = 0;
		break;
	case M2:
		index = 1;
		break;
	default:
		break;
	}

	// Init motor driver
	palSetPad(DRIVER_GPIO, DRIVER_RESET);
	chThdSleepMilliseconds(500);
	pwmStart(&PWM_DRIVER, &pwmcfg);

	node.subscribe(speed_sub, "speed2");

	switch (stm32_id8()) {
	case M1:
		node.subscribe(enc_sub, "encoder1");
		break;
	case M2:
		node.subscribe(enc_sub, "encoder2");
		break;
	default:
		node.subscribe(enc_sub, "encoder");
		break;
	}

	for (;;) {
		if (node.spin(Time::ms(100))) {
			if (speed_sub.fetch(msgp)) {
				speed_pid.set(msgp->value[index]);
				last_setpoint = Time::now();
				speed_sub.release(*msgp);

				palTogglePad(LED3_GPIO, LED3);

			} else if (Time::now() - last_setpoint > Time::ms(100)) {
					speed_pid.set(0);
			}
		} else {
			// Stop motor if no messages for 100 ms
			pwm_lld_disable_channel(&PWM_DRIVER, 0);
			pwm_lld_disable_channel(&PWM_DRIVER, 1);

			palTogglePad(LED4_GPIO, LED4);
		}
	}

	return CH_SUCCESS;
}

msg_t pid3_node(void * arg) {
	Node node("pid");
	Subscriber<Speed3Msg, 5> speed_sub;
	Subscriber<tEncoderMsg, 5> enc_sub(enc_callback);
	Speed3Msg * msgp;
	Time last_setpoint(0);

	(void) arg;

	chRegSetThreadName("pid");

//	speed_pid.config(450.0, 0.125, 0.0, 0.01, -4095.0, 4095.0); /* Robocom */
	speed_pid.config(250.0, 0.0, 0.0, 0.01, -4095.0, 4095.0); /* Triskar */

	switch (stm32_id8()) {
	case M1:
		index = 0;
		break;
	case M2:
		index = 1;
		break;
	case M3:
		index = 2;
		break;
	default:
		break;
	}

	// Init motor driver
	palSetPad(DRIVER_GPIO, DRIVER_RESET);
	chThdSleepMilliseconds(500);
	pwmStart(&PWM_DRIVER, &pwmcfg);

	node.subscribe(speed_sub, "speed3");

	switch (stm32_id8()) {
	case M1:
		node.subscribe(enc_sub, "encoder1");
		break;
	case M2:
		node.subscribe(enc_sub, "encoder2");
		break;
	case M3:
		node.subscribe(enc_sub, "encoder3");
		break;
	default:
		node.subscribe(enc_sub, "encoder");
		break;
	}

	for (;;) {
		if (node.spin(Time::ms(100))) {
			if (speed_sub.fetch(msgp)) {
				speed_pid.set(msgp->value[index]);
				last_setpoint = Time::now();
				speed_sub.release(*msgp);

				palTogglePad(LED3_GPIO, LED3);

			} else if (Time::now() - last_setpoint > Time::ms(100)) {
					speed_pid.set(0);
			}
		} else {
			// Stop motor if no messages for 100 ms
			pwm_lld_disable_channel(&PWM_DRIVER, 0);
			pwm_lld_disable_channel(&PWM_DRIVER, 1);

			palTogglePad(LED4_GPIO, LED4);
		}
	}

	return CH_SUCCESS;
}

/*===========================================================================*/
/* QEI nodes.                                                                */
/*===========================================================================*/

/*
 * QEI publisher node
 */

msg_t qeipub_node(void *arg) {
	Node node("qeipub");
	Publisher<tQEIMsg> qei_pub;
	systime_t time;
	tQEIMsg *msgp;

	(void) arg;
	chRegSetThreadName("qeipub");

	qeiStart(&QEI_DRIVER, &qeicfg);
	qeiEnable (&QEI_DRIVER);

	switch (stm32_id8()) {
	case M1:
		node.advertise(qei_pub, "qei1");
		break;
	case M2:
		node.advertise(qei_pub, "qei2");
		break;
	case M3:
		node.advertise(qei_pub, "qei3");
		break;
	default:
		node.advertise(qei_pub, "qei");
		break;
	}

	for (;;) {
		time = chTimeNow();
		int16_t delta = qeiUpdate(&QEI_DRIVER);

		if (qei_pub.alloc(msgp)) {
			msgp->timestamp.sec = 0;
			msgp->timestamp.nsec = 0;
			msgp->delta = delta;
			qei_pub.publish(*msgp);
		}

		time += MS2ST(50);
		chThdSleepUntil(time);
	}

	return CH_SUCCESS;
}

msg_t encoder_node(void *arg) {
	Node node("encoder");
	Publisher<tEncoderMsg> enc_pub;
	systime_t time;
	tEncoderMsg *msgp;

	(void) arg;
	chRegSetThreadName("encoder");

	qeiStart(&QEI_DRIVER, &qeicfg);
	qeiEnable (&QEI_DRIVER);


	switch (stm32_id8()) {
	case M1:
		node.advertise(enc_pub, "encoder1");
		break;
	case M2:
		node.advertise(enc_pub, "encoder2");
		break;
	case M3:
		node.advertise(enc_pub, "encoder3");
		break;
	default:
		node.advertise(enc_pub, "encoder");
		break;
	}

	for (;;) {
		time = chTimeNow();

		if (enc_pub.alloc(msgp)) {
			msgp->timestamp.sec = chTimeNow();
			msgp->timestamp.nsec = chTimeNow();
			msgp->delta = T2R(qeiUpdate(&QEI_DRIVER) * 100);
			enc_pub.publish(*msgp);
		}

		time += MS2ST(10);
		chThdSleepUntil(time);
	}

	return CH_SUCCESS;
}

} /* namespace r2p */
