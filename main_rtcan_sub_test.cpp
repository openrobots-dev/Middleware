
#include "ch.h"
#include "hal.h"

#include <r2p/common.hpp>
#include <r2p/Middleware.hpp>
#include <r2p/Node.hpp>
#include <r2p/Topic.hpp>
#include <r2p/Publisher.hpp>
#include <r2p/Subscriber.hpp>
#include <r2p/Mutex.hpp>
#include <r2p/NamingTraits.hpp>
#include "DebugTransport.hpp"
#include "RTCANTransport.hpp"

#include <cstdio>
#include <cstdlib>
#include <cstring>

#define WA_SIZE_256B      THD_WA_SIZE(256)
#define WA_SIZE_512B      THD_WA_SIZE(512)
#define WA_SIZE_1K        THD_WA_SIZE(1024)
#define WA_SIZE_2K        THD_WA_SIZE(2048)

struct LedMsg : public r2p::Message {
	uint32_t led;
	uint32_t value;
	uint32_t cnt;
} R2P_PACKED;

void *__dso_handle;

extern "C" void __cxa_pure_virtual() {}

static WORKING_AREA(wa_info, 1024);

r2p::Bootloader r2p::Bootloader::instance(NULL);
r2p::Middleware r2p::Middleware::instance("IMU_0", "BOOT_IMU_0");

static char dbgtra_namebuf[64];

// Debug transport
static r2p::DebugTransport dbgtra(reinterpret_cast<BaseChannel *>(&SD2),
  dbgtra_namebuf);

static WORKING_AREA(wa_rx_dbgtra, 1024);
static WORKING_AREA(wa_tx_dbgtra, 1024);

// RTCAN transport
static r2p::RTCANTransport rtcantra(RTCAND1);


bool callback(const LedMsg &msg) {
	if (msg.value) {
		palClearPad(LED_GPIO, msg.led);
	} else {
		palSetPad(LED_GPIO, msg.led);
	}
	return true;
}

msg_t LedSub(void * arg) {
  LedMsg sub_msgbuf[5], *sub_queue[5];
  r2p::Node node("LedSub");
  r2p::Subscriber<LedMsg> sub(sub_queue, 5, callback);
  char * tn = (char *)arg;

  chRegSetThreadName("LedSub");

  node.subscribe(sub, tn, sub_msgbuf);

  for (;;) {
    node.spin(1000);
  }
  return CH_SUCCESS;
}


/*
 * LED blinker thread, times are in milliseconds.
 */
static WORKING_AREA(wa_blinker_thread, 128);
static msg_t blinker_thread(void *arg) {

	(void) arg;
	chRegSetThreadName("blinker");

	while (TRUE) {
		switch (RTCAND1.state) {
		case RTCAN_MASTER:
			palClearPad(LED_GPIO, LED1);
			chThdSleepMilliseconds(200);
			palSetPad(LED_GPIO, LED1);
			chThdSleepMilliseconds(100);
			palClearPad(LED_GPIO, LED1);
			chThdSleepMilliseconds(200);
			palSetPad(LED_GPIO, LED1);
			chThdSleepMilliseconds(500);
			break;
		case RTCAN_SYNCING:
			palTogglePad(LED_GPIO, LED1);
			chThdSleepMilliseconds(100);
			break;
		case RTCAN_SLAVE:
			palTogglePad(LED_GPIO, LED1);
			chThdSleepMilliseconds(500);
			break;
		case RTCAN_ERROR:
			palTogglePad(LED_GPIO, LED4);
			chThdSleepMilliseconds(200);
			break;
		default:
			chThdSleepMilliseconds(100);
			break;
		}
	}

	return 0;
}

extern "C" {
int main(void) {

  Thread *tp1, *tp2, *tp3; (void)tp1, (void)tp2, (void)tp3;
  static const RTCANConfig rtcan_config = {1000000, 100, 60};

  halInit();
  chSysInit();

  sdStart(&SD2, NULL);

  /*
   * Creates the blinker thread.
   */
  chThdCreateStatic(wa_blinker_thread, sizeof(wa_blinker_thread), NORMALPRIO, blinker_thread, NULL);

  chThdSetPriority(HIGHPRIO);
  r2p::Middleware::instance.initialize(wa_info, sizeof(wa_info),
                                       r2p::Thread::IDLE);

  rtcantra.initialize(rtcan_config);

/*
  dbgtra.initialize(wa_rx_dbgtra, sizeof(wa_rx_dbgtra), r2p::Thread::LOWEST + 11,
                    wa_tx_dbgtra, sizeof(wa_tx_dbgtra), r2p::Thread::LOWEST + 10);
*/

//  chThdCreateFromHeap(NULL, WA_SIZE_1K, NORMALPRIO + 1, LedSub, (void*)"led2");
//  chThdCreateFromHeap(NULL, WA_SIZE_1K, NORMALPRIO + 1, LedSub, (void*)"led3");
  chThdCreateFromHeap(NULL, WA_SIZE_1K, NORMALPRIO + 1, LedSub, (void*)"led2");

  chThdSetPriority(IDLEPRIO);
  for (;;) {
    chThdYield();
  }
  return CH_SUCCESS;
}
}
