#include "ch.h"
#include "hal.h"
#include <stdint.h>

typedef struct {
   uint32_t address;
   char name[32];
   void *dummy;
} DummyInfo;

const DummyInfo app_config =  { 0xEFBEADDE, "DEADBEEF", (void*)0x37133713 };


msg_t app_thread(void *arg) {

    (void)arg;

	for (;;) {
        palTogglePad(LED_GPIO, LED4);
		chThdSleepMilliseconds(500);
	}
	return CH_SUCCESS;
}
