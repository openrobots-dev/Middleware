#include "ch.h"
#include "hal.h"

#include <r2p/common.hpp>
#include <r2p/Middleware.hpp>
#include <r2p/Node.hpp>
#include <r2p/Publisher.hpp>
#include <r2p/Subscriber.hpp>

#include <r2p/msg/led.hpp>

namespace r2p {

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
		chSysHalt();
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
		chSysHalt();
		return 0;
	}
}

/*
 * Led publisher node
 */
msg_t ledpub_node(void *arg) {
	Node node("ledpub");
	Publisher<LedMsg> led_pub;
	uint8_t led = *(uint8_t *) arg;
	uint32_t toggle = 0;
#if R2P_LEDDEBUG
	uint32_t cnt = 0;
#endif

	R2P_ASSERT(led);

	node.advertise(led_pub, "leds");

	for (;;) {
		LedMsg *msgp;
		if (led_pub.alloc(msgp)) {
			msgp->led = led;
			msgp->value = toggle;
			toggle ^= 1;
#if R2P_LEDDEBUG
			msgp->cnt = cnt;
			cnt++;
#endif
			if (!led_pub.publish(*msgp)) {
				R2P_ASSERT(false);
			}

		}

		r2p::Thread::sleep(r2p::Time::ms(500)); //TODO: Node::sleep()
	}
	return CH_SUCCESS;
}

/*
 * Led subscriber node
 */

bool callback(const LedMsg &msg) {

	palWritePad((GPIO_TypeDef *)led2gpio(msg.led), led2pin(msg.led), msg.value);

	return true;
}

msg_t ledsub_node(void * arg) {
	LedMsg sub_msgbuf[5], *sub_queue[5];
	Node node("ledsub");
    Subscriber<LedMsg> sub(sub_queue, 5, callback);

    (void)arg;

	node.subscribe(sub, "leds", sub_msgbuf);

	for (;;) {
		if (!node.spin(Time::ms(1000))) {
			palTogglePad((GPIO_TypeDef *)led2gpio(4), led2pin(4));
		} else {
			palSetPad((GPIO_TypeDef *)led2gpio(4), led2pin(4));
		}
	}
	return CH_SUCCESS;
}

} /* namespace r2p */

