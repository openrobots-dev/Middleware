
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

#include <cstdio>
#include <cstdlib>
#include <cstring>


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

static WORKING_AREA(wa1, 1024);
static WORKING_AREA(wa2, 1024);
static WORKING_AREA(wa3, 1024);

static r2p::Mutex print_lock;

static r2p::Topic test_topic("test", sizeof(Uint32Msg));

static char dbgtra_namebuf[64];

static r2p::DebugTransport dbgtra(
  "debug", reinterpret_cast<BaseChannel *>(&SD2),
  dbgtra_namebuf, sizeof(dbgtra_namebuf)
);

static WORKING_AREA(wa_rx_dbgtra, 1024);
static WORKING_AREA(wa_tx_dbgtra, 1024);

size_t num_msgs = 0;
systime_t start_time, cur_time;


msg_t Thread1(void *) {

  chRegSetThreadName("Thread1");

  r2p::Node node("Node1");
  r2p::Publisher<Uint32Msg> pub1;

  r2p::Middleware::instance.add(node);
  node.advertise(pub1, "test");

  chThdSleepMilliseconds(1000);
  start_time = chTimeNow();

  for (;;) {
    chThdSleepMilliseconds(250);
    cur_time = chTimeNow();
    Uint32Msg *msgp;
    if (pub1.alloc(msgp)) {
      msgp->value = 1;
      if (!pub1.publish(*msgp)) {
        chSysHalt();
      }
      palTogglePad(LED_GPIO, LED1);
    } else {
      palTogglePad(LED_GPIO, LED4);
    }
  }
  return CH_SUCCESS;
}


bool blinkled2(const Uint32Msg &msg) {

  (void)msg;
  palTogglePad(LED_GPIO, LED2);
  ++num_msgs;
  return true;
}


bool blinkled3(const Uint32Msg &msg) {

  (void)msg;
  palTogglePad(LED_GPIO, LED3);
  return true;
}


msg_t Thread2(void *) {

  chRegSetThreadName("Thread2");

  Uint32Msg sub2_msgbuf[5], *sub2_queue[5];
  r2p::Node node("Node2");
  r2p::Subscriber<Uint32Msg> sub2(sub2_queue, 5, blinkled2);

  r2p::Middleware::instance.add(node);
  node.subscribe(sub2, "test", sub2_msgbuf);

  for (;;) {
    if (!node.spin()) {
      palTogglePad(LED_GPIO, LED4);
    }
  }
  return CH_SUCCESS;
}


msg_t Thread3(void *) {

  chRegSetThreadName("Thread3");

  Uint32Msg sub3_msgbuf[5], *sub3_queue[5];
  r2p::Node node("Node3");
  r2p::Subscriber<Uint32Msg> sub3(sub3_queue, 5, blinkled3);

  r2p::Middleware::instance.add(node);
  node.subscribe(sub3, "asdf", sub3_msgbuf);

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

  halInit();
  chSysInit();

  sdStart(&SD2, NULL);

  chThdSetPriority(HIGHPRIO);
  r2p::Middleware::instance.initialize(wa_info, sizeof(wa_info),
                                       r2p::Thread::IDLE);
  r2p::Middleware::instance.add(test_topic);

  dbgtra.initialize(wa_rx_dbgtra, sizeof(wa_rx_dbgtra), r2p::Thread::LOWEST + 11,
                    wa_tx_dbgtra, sizeof(wa_tx_dbgtra), r2p::Thread::LOWEST + 10);

  tp1 = chThdCreateStatic(wa1, sizeof(wa1), NORMALPRIO + 0, Thread1, NULL);
  tp2 = chThdCreateStatic(wa2, sizeof(wa2), NORMALPRIO + 1, Thread2, NULL);
  tp3 = chThdCreateStatic(wa3, sizeof(wa3), NORMALPRIO + 2, Thread3, NULL);

  chThdSetPriority(IDLEPRIO);
  for (;;) {
    chThdYield();
  }
  return CH_SUCCESS;
}
}
