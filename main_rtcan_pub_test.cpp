
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


struct Uint32Msg : public r2p::Message {
  uint32_t value;
} R2P_PACKED;


struct FloatMsg : public r2p::Message {
  float value;
} R2P_PACKED;


void *__dso_handle;

extern "C" void __cxa_pure_virtual() {}

static WORKING_AREA(wa_info, 1024);

r2p::Middleware r2p::Middleware::instance("IMU_0", "BOOT_IMU_0");

static WORKING_AREA(wa1, 1024);
static WORKING_AREA(wa2, 1024);
static WORKING_AREA(wa3, 1024);

static r2p::Mutex print_lock;

static r2p::Topic test_topic("test", sizeof(Uint32Msg));

static char dbgtra_namebuf[64];

// Debug transport
static r2p::DebugTransport dbgtra(reinterpret_cast<BaseChannel *>(&SD2),
  dbgtra_namebuf);

static WORKING_AREA(wa_rx_dbgtra, 1024);
static WORKING_AREA(wa_tx_dbgtra, 1024);

// RTCAN transport
static r2p::RTCANTransport rtcantra(RTCAND1);

size_t num_msgs = 0;
systime_t start_time, cur_time;

r2p::Middleware * mw;


msg_t PubThd(void *) {

  chRegSetThreadName("PubThd");

  r2p::Node node("Pub");
  r2p::Publisher<Uint32Msg> pub1;

  node.advertise(pub1, "test");

  chThdSleepMilliseconds(1000);
  start_time = chTimeNow();

  for (;;) {
    chThdSleepMilliseconds(250);
    cur_time = chTimeNow();
    Uint32Msg *msgp;
    if (pub1.alloc(msgp)) {
      msgp->value = 0xAAAAAAAA;
      if (!pub1.publish(*msgp)) {
        chSysHalt();
      }
    } else {
      palTogglePad(LED_GPIO, LED4);
    }
  }
  return CH_SUCCESS;
}


bool callback_2(const Uint32Msg &msg) const {
  (void)msg;
  palTogglePad(LED_GPIO, LED2);
  return true;
}


msg_t SubThd(void *) {

  chRegSetThreadName("SubThd");

  Uint32Msg sub2_msgbuf[5], *sub2_queue[5];
  r2p::Node node("Sub");
  r2p::Subscriber<Uint32Msg> sub2(sub2_queue, 5, callback_2);

  node.subscribe(sub2, "test", sub2_msgbuf);

  for (;;) {
    if (!node.spin()) {
      palTogglePad(LED_GPIO, LED4);
    }
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
  static const RTCANConfig rtcan_config = {1000000, 100, 60};

  halInit();
  chSysInit();

  sdStart(&SD2, NULL);

  /*
   * Creates the blinker thread.
   */
  chThdCreateStatic(wa_blinker_thread, sizeof(wa_blinker_thread), NORMALPRIO, blinker_thread, NULL);


  r2p::Thread::set_priority(r2p::Thread::HIGHEST);
  r2p::Middleware::instance.initialize(wa_info, sizeof(wa_info),
                                       r2p::Thread::IDLE);
  r2p::Middleware::instance.add(test_topic);

  rtcantra.initialize(rtcan_config);

  dbgtra.initialize(wa_rx_dbgtra, sizeof(wa_rx_dbgtra), r2p::Thread::LOWEST + 11,
                    wa_tx_dbgtra, sizeof(wa_tx_dbgtra), r2p::Thread::LOWEST + 10);

  chThdCreateFromHeap(NULL, WA_SIZE_1K, NORMALPRIO + 0, PubThd, NULL);
  chThdCreateFromHeap(NULL, WA_SIZE_1K, NORMALPRIO + 1, SubThd, NULL);

  r2p::Thread::set_priority(r2p::Thread::IDLE);
  for (;;) {
//    r2p::Thread::yield();
    r2p::Thread::sleep(100);
  }
  return CH_SUCCESS;
}
}
