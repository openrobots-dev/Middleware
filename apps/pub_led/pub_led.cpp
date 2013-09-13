#include "ch.h"
#include "hal.h"

#include <r2p/Middleware.hpp>

using namespace r2p;


struct AppConfig {
    uint16_t l33t;
};


const AppConfig app_config = { 0x1337 };


extern "C"
msg_t app_thread(void *arg) {

    (void)arg;

    if (r2p::Middleware::instance.is_stopped()) {
        palTogglePad(LED_GPIO, LED4);
    }

	for (;;) {
        palTogglePad(LED_GPIO, LED4);
		chThdSleepMilliseconds(500);
	}
	return CH_SUCCESS;
}

