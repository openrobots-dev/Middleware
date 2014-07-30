#include "ch.h"
#include "hal.h"

#include <r2p/common.hpp>
#include <r2p/Middleware.hpp>
#include <r2p/Node.hpp>
#include <r2p/Publisher.hpp>
#include <r2p/Subscriber.hpp>

#include <r2p/node/led.hpp>
#include <r2p/msg/led.hpp>

namespace r2p {

ledpub_conf default_ledpub_conf = { "led2", 2 };
ledsub_conf default_ledsub_conf = { "led2" };

/*
 * Utility functions.
 */
ioportid_t led2gpio(unsigned led_id) {

	switch (led_id) {
	case 1:
		return LED1_GPIO;
	case 2:
		return LED2_GPIO;
	case 3:
		return LED3_GPIO;
	case 4:
		return LED4_GPIO;
	default:
		chSysHalt()
		;
		return 0;
	}
}

unsigned led2pin(unsigned led_id) {

	switch (led_id) {
	case 1:
		return LED1;
	case 2:
		return LED2;
	case 3:
		return LED3;
	case 4:
		return LED4;
	default:
		chSysHalt()
		;
		return 0;
	}
}

/*
 * Led publisher node
 */
msg_t ledpub_node(void * arg) {
	ledpub_conf * conf = reinterpret_cast<ledpub_conf *>(arg);
	Node node("ledpub");
	Publisher<LedMsg> led_pub;
	uint8_t led;
	uint32_t toggle = 0;

	if (conf == NULL) conf = &default_ledpub_conf;

	led = conf->led;

	node.advertise(led_pub, conf->topic);

	for (;;) {
		LedMsg *msgp;
		if (led_pub.alloc(msgp)) {
			msgp->led = led;
			msgp->value = toggle;
			toggle ^= 1;
			if (!led_pub.publish(*msgp)) {
				R2P_ASSERT(false);
			}

		}

		chThdSleepMilliseconds(500); //TODO: Node::sleep()
	}
	return CH_SUCCESS;
}

/*
 * Led subscriber node
 */

bool callback(const LedMsg &msg) {

	palWritePad((GPIO_TypeDef *)led2gpio(msg.led), led2pin(msg.led), msg.value); palSetPad((GPIO_TypeDef *)led2gpio(4), led2pin(4));

	return true;
}

msg_t ledsub_node(void * arg) {
	ledsub_conf * conf = reinterpret_cast<ledsub_conf *>(arg);
	Node node("ledsub");
	Subscriber<LedMsg, 5> sub(callback);

	if (conf == NULL) conf = &default_ledsub_conf;

	node.subscribe(sub, conf->topic);

	for (;;) {
		if (!node.spin(Time::ms(1000))) {
			palTogglePad((GPIO_TypeDef *)led2gpio(4), led2pin(4));
		}
	}
	return CH_SUCCESS;
}

} /* namespace r2p */

