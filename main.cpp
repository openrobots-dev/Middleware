
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
#include <r2p/Bootloader.hpp>
#include "DebugTransport.hpp"

#include <cstdio>
#include <cstdlib>
#include <cstring>

#ifndef R2P_MODULE_NAME
#define R2P_MODULE_NAME "R2PMODX"
#endif


struct Uint32Msg : public r2p::Message {
  uint32_t value;
} R2P_PACKED;


struct FloatMsg : public r2p::Message {
  float value;
} R2P_PACKED;


void *__dso_handle;

extern "C" void __cxa_pure_virtual() { chSysHalt(); }

static WORKING_AREA(wa_info, 2048);

r2p::Middleware r2p::Middleware::instance(R2P_MODULE_NAME,
                                          "BOOT_"R2P_MODULE_NAME);

static WORKING_AREA(wa1, 1024);
static WORKING_AREA(wa2, 1024);
static WORKING_AREA(wa3, 1024);

static r2p::Topic test_topic("test", sizeof(Uint32Msg));

static char dbgtra_namebuf[64];

static r2p::DebugTransport dbgtra(reinterpret_cast<BaseChannel *>(&SD2),
                                  dbgtra_namebuf);

static WORKING_AREA(wa_rx_dbgtra, 1024);
static WORKING_AREA(wa_tx_dbgtra, 1024);

size_t num_msgs = 0;
systime_t start_time, cur_time;


msg_t Thread1(void *) {

  r2p::Node node("Node1");
  r2p::Publisher<Uint32Msg> pub1;

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


bool callback_2(const Uint32Msg &msg) {
  (void)msg;
  palTogglePad(LED_GPIO, LED2);
  ++num_msgs;
  return true;
}


static unsigned remote_msg_count = 0;

bool callback_3(const Uint32Msg &msg) {
    (void)msg;
    ++remote_msg_count;
    palTogglePad(LED_GPIO, LED3);
    return true;
}


msg_t Thread2(void *) {

  Uint32Msg sub2_msgbuf[5], *sub2_queue[5];
  r2p::Node node("Node2");
  r2p::Subscriber<Uint32Msg> sub2(sub2_queue, 5, callback_2);

  node.subscribe(sub2, "test", sub2_msgbuf);

  for (;;) {
    if (!node.spin()) {
      palTogglePad(LED_GPIO, LED4);
    }
  }
  return CH_SUCCESS;
}


msg_t Thread3(void *) {

  Uint32Msg sub3_msgbuf[5], *sub3_queue[5];
  r2p::Node node("Node3");
  r2p::Subscriber<Uint32Msg> sub3(sub3_queue, 5, callback_3);

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

  halInit();
  chSysInit();

  sdStart(&SD2, NULL);

  r2p::Thread::set_priority(r2p::Thread::HIGHEST);
  r2p::Middleware::instance.initialize(wa_info, sizeof(wa_info),
                                       r2p::Thread::LOWEST);
  r2p::Middleware::instance.add(test_topic);

  dbgtra.initialize(wa_rx_dbgtra, sizeof(wa_rx_dbgtra), r2p::Thread::LOWEST + 11,
                    wa_tx_dbgtra, sizeof(wa_tx_dbgtra), r2p::Thread::LOWEST + 10);

  //r2p::Thread::create_static(wa3, sizeof(wa3), r2p::Thread::NORMAL - 2, Thread3, NULL, "Thread3");
  //r2p::Thread::create_static(wa2, sizeof(wa2), r2p::Thread::NORMAL + 1, Thread2, NULL, "Thread2");
  //r2p::Thread::create_static(wa1, sizeof(wa1), r2p::Thread::NORMAL + 0, Thread1, NULL, "Thread1");

  for (;;) {
    palTogglePad(LED_GPIO, LED1);
    r2p::Thread::sleep(r2p::Time::ms(500));
  }
  return CH_SUCCESS;
}
}
