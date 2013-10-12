#include "ch.h"
#include "hal.h"

#include "config.h"

#include <r2p/common.hpp>
#include <r2p/Middleware.hpp>
#include <r2p/Node.hpp>
#include <r2p/Publisher.hpp>
#include <r2p/Subscriber.hpp>

#include <r2p/msg/motor.hpp>

namespace r2p {

/*===========================================================================*/
/* Utility functions.                                                        */
/*===========================================================================*/

uint8_t stm32_id8(void) {
	const unsigned long * uid = (const unsigned long *) 0x1FFFF7E8;

	return (uid[2] & 0xFF);
}

/*===========================================================================*/
/* PWM nodes.                                                                */
/*===========================================================================*/

/*
 * PWM subscriber node
 */
bool callback(const PWM2Msg &msg) {
	int16_t pwm;

	switch (stm32_id8()) {
	case 72:
		pwm = msg.pwm1;
		break;
	case 73:
		pwm = msg.pwm2;
		break;
	default:
		pwm = 0;
		return true;
	}

	chSysLock();

	if (pwm > 0) {
		pwm_lld_enable_channel(&PWM_DRIVER, 0, pwm);
		pwm_lld_enable_channel(&PWM_DRIVER, 1, 0);
	} else {
		pwm_lld_enable_channel(&PWM_DRIVER, 0, 0);
		pwm_lld_enable_channel(&PWM_DRIVER, 1, -pwm);
	}

#if R2P_PWMDEBUG
	if (pwm == 0) {
		palSetPad(LED2_GPIO, LED3);
	} else {
		palTogglePad(LED2_GPIO, LED3);
	}
#endif

	chSysUnlock();

	return true;
}

msg_t pwm2sub_node(void * arg) {
	PWM2Msg sub_msgbuf[5], *sub_queue[5];
	Node node("pwm2sub");
	Subscriber<PWM2Msg> sub(sub_queue, 5, callback);

	(void) arg;

	chRegSetThreadName("pwm2sub");

	palSetPad(DRIVER_GPIO, DRIVER_RESET);
	chThdSleepMilliseconds(500);
	pwmStart(&PWM_DRIVER, &pwmcfg);

	node.subscribe(sub, "pwm2", sub_msgbuf);

	for (;;) {
		if (!node.spin(Time::ms(100))) {
			// Stop motor if no messages for 100 ms
			chSysLock();
			pwm_lld_enable_channel(&PWM_DRIVER, 0, 0);
			pwm_lld_enable_channel(&PWM_DRIVER, 0, 0);
			chSysUnlock();
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
	Publisher<QEIMsg> qei_pub;
	systime_t time;
	QEIMsg *msgp;

	(void)arg;
	chRegSetThreadName("qeipub");

	qeiStart(&QEI_DRIVER, &qeicfg);
	qeiEnable (&QEI_DRIVER);

	node.advertise(qei_pub, "qei");

	for (;;) {
		time = chTimeNow();

		if (qei_pub.alloc(msgp)) {
			msgp->delta = qeiUpdate(&QEI_DRIVER);
			qei_pub.publish(*msgp);
		}

		time += MS2ST(50);
		chThdSleepUntil(time);
	}

	return CH_SUCCESS;
}

} /* namespace r2p */
