
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


struct Uint32Msg : public r2p::BaseMessage {
  uint32_t value;
};


struct FloatMsg : public r2p::BaseMessage {
  float value;
};


void *__dso_handle;

extern "C" void __cxa_pure_virtual() {}

static WORKING_AREA(wa_info, 1024);

r2p::Middleware r2p::Middleware::instance("IMU_0", "BOOT_IMU_0");

static r2p::Mutex print_lock;

static r2p::Topic test_topic("test", sizeof(Uint32Msg));

static char dbgtra_namebuf[64];

// Debug transport
static r2p::DebugTransport dbgtra(
  "debug", reinterpret_cast<BaseChannel *>(&SD2),
  dbgtra_namebuf, sizeof(dbgtra_namebuf)
);

static WORKING_AREA(wa_rx_dbgtra, 1024);
static WORKING_AREA(wa_tx_dbgtra, 1024);

// RTCAN transport
static r2p::RTCANTransport rtcantra("rtcan", RTCAND1);

size_t num_msgs = 0;
systime_t start_time, cur_time;

class BlinkLed2 : public r2p::Subscriber<Uint32Msg>::Callback {
  bool action(const Uint32Msg &msg) const {
    (void)msg;
    palTogglePad(LED_GPIO, LED2);
    ++num_msgs;
    return true;
  }
};

msg_t SubThd(void *) {

  chRegSetThreadName("SubThd");

  Uint32Msg sub2_msgbuf[5], *sub2_queue[5];
  r2p::Node node("Sub");
  BlinkLed2 blinker;
  r2p::Subscriber<Uint32Msg> sub2(sub2_queue, 5, &blinker);

  r2p::Middleware::instance.add(node);
  node.subscribe(sub2, "test", sub2_msgbuf);

  for (;;) {
    if (!node.spin()) {
      palTogglePad(LED_GPIO, LED4);
    }
  }
  return CH_SUCCESS;
}



extern "C" {
int main(void) {

  Thread *tp1, *tp2, *tp3; (void)tp1, (void)tp2, (void)tp3;
  static const RTCANConfig rtcan_config = {1000000, 100, 60};

  halInit();
  chSysInit();

  sdStart(&SD2, NULL);

  chThdSetPriority(HIGHPRIO);
  r2p::Middleware::instance.initialize(wa_info, sizeof(wa_info),
                                       r2p::Thread::IDLE);
  r2p::Middleware::instance.add(test_topic);

  rtcantra.initialize(rtcan_config);

/*
  dbgtra.initialize(wa_rx_dbgtra, sizeof(wa_rx_dbgtra), r2p::Thread::LOWEST + 11,
                    wa_tx_dbgtra, sizeof(wa_tx_dbgtra), r2p::Thread::LOWEST + 10);
*/

  chThdCreateFromHeap(NULL, WA_SIZE_1K, NORMALPRIO + 1, SubThd, NULL);

  chThdSetPriority(IDLEPRIO);
  for (;;) {
    chThdYield();
  }
  return CH_SUCCESS;
}
}
