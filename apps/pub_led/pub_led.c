#include "ch.h"
#include "hal.h"


struct AppConfig {
    uint16_t l33t;
};


const struct AppConfig app_config = { 0x1337 };


msg_t app_thread(void *arg) {

    (void)arg;

	for (;;) {
        palTogglePad(LED_GPIO, LED4);
		chThdSleepMilliseconds(500);
	}
	return CH_SUCCESS;
}

