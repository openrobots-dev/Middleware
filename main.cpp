
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
#include <r2p/transport/DebugTransport.hpp>
#include <r2p/transport/RTCANTransport.hpp>

#include <r2p/node/led.hpp>
#include <r2p/msg/motor.hpp>

#include <cstdio>
#include <cstdlib>
#include <cstring>

#ifndef R2P_MODULE_NAME
#define R2P_MODULE_NAME "IMU_GW"
#endif

#ifndef DEBUGTRA
#define DEBUGTRA    1
#endif

#ifndef RTCANTRA
#define RTCANTRA    1
#endif


struct TestMsg : public r2p::Message {
  long id;
  long value;
} R2P_PACKED;


extern "C" {
  void *__dso_handle;
  void __cxa_pure_virtual() { chSysHalt(); }
  void _exit(int) { chSysHalt(); for (;;) {} }
  int _kill(int, int) { chSysHalt(); return -1; }
  int _getpid() { return 1; }
} // extern "C"


#define BOOT_STACKLEN   1024

#if R2P_USE_BRIDGE_MODE
enum { PUBSUB_BUFFER_LENGTH = 16 };
r2p::Middleware::PubSubStep pubsub_buf[PUBSUB_BUFFER_LENGTH];
#endif

static WORKING_AREA(wa_info, 1024);

r2p::Middleware r2p::Middleware::instance(
  R2P_MODULE_NAME, "BOOT_"R2P_MODULE_NAME
#if R2P_USE_BRIDGE_MODE
, pubsub_buf, PUBSUB_BUFFER_LENGTH
#endif
);

static WORKING_AREA(wa1, 1024);
static WORKING_AREA(wa2, 1024);
static WORKING_AREA(wa3, 1024);

#if DEBUGTRA
static char dbgtra_namebuf[64];
static r2p::DebugTransport dbgtra("SD2", reinterpret_cast<BaseChannel *>(&SD2),
                                  dbgtra_namebuf);
static WORKING_AREA(wa_rx_dbgtra, 1024);
static WORKING_AREA(wa_tx_dbgtra, 1024);
#endif // DEBUGTRA

#if RTCANTRA
static r2p::RTCANTransport rtcantra(RTCAND1);
static RTCANConfig rtcan_config = { 1000000, 100, 60 };
#endif // RTCANTRA

size_t num_msgs = 0;
systime_t start_time, cur_time;


msg_t Thread1(void *) {

  r2p::Node node("Node1");
  r2p::Publisher<TestMsg> pub1;

  node.advertise(pub1, "test");

  chThdSleepMilliseconds(1000);
  start_time = chTimeNow();

  for (;;) {
    chThdSleepMilliseconds(250);
    cur_time = chTimeNow();
    TestMsg *msgp;
    if (pub1.alloc(msgp)) {
      msgp->id = 0xDEADBEEF;
      msgp->value = 0x1337B00B;
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


bool callback_2(const TestMsg &msg) {
  (void)msg;
  palTogglePad(LED_GPIO, LED2);
  ++num_msgs;
  return true;
}


static unsigned remote_msg_count = 0;

bool callback_3(const TestMsg &msg) {
    (void)msg;
    ++remote_msg_count;
    palTogglePad(LED_GPIO, LED3);
    return true;
}


msg_t Thread2(void *) {

  TestMsg sub2_msgbuf[5], *sub2_queue[5];
  r2p::Node node("Node2");
  r2p::Subscriber<TestMsg> sub2(sub2_queue, 5, callback_2);

  node.subscribe(sub2, "test", sub2_msgbuf);

  for (;;) {
    if (!node.spin()) {
      palTogglePad(LED_GPIO, LED4);
    }
  }
  return CH_SUCCESS;
}


msg_t Thread3(void *) {

  TestMsg sub3_msgbuf[5], *sub3_queue[5];
  r2p::Node node("Node3");
  r2p::Subscriber<TestMsg> sub3(sub3_queue, 5, callback_3);

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

  r2p::Thread::set_priority(r2p::Thread::HIGHEST);

  void *boot_stackp = NULL;
  if (r2p::Middleware::is_bootloader_mode()) {
    uint8_t *stackp = new uint8_t[BOOT_STACKLEN + sizeof(stkalign_t)];
    R2P_ASSERT(stackp != NULL);
    stackp += (sizeof(stkalign_t) - reinterpret_cast<uintptr_t>(stackp)) %
              sizeof(stkalign_t);
    boot_stackp = stackp;
  }

  r2p::Middleware::instance.initialize(
    wa_info, sizeof(wa_info), r2p::Thread::LOWEST,
    boot_stackp, BOOT_STACKLEN, r2p::Thread::LOWEST
  );

#if DEBUGTRA
  sdStart(&SD2, NULL);
  dbgtra.initialize(wa_rx_dbgtra, sizeof(wa_rx_dbgtra), r2p::Thread::LOWEST + 11,
                    wa_tx_dbgtra, sizeof(wa_tx_dbgtra), r2p::Thread::LOWEST + 10);
#endif

#if RTCANTRA
  rtcantra.initialize(rtcan_config);
#endif

  r2p::Middleware::instance.start();

  (void)wa1; (void)wa2; (void)wa3;
//  r2p::Thread::create_static(wa3, sizeof(wa3), r2p::Thread::NORMAL - 2, Thread3, NULL, "Thread3");
//  r2p::Thread::create_static(wa2, sizeof(wa2), r2p::Thread::NORMAL + 1, Thread2, NULL, "Thread2");
//  r2p::Thread::create_static(wa1, sizeof(wa1), r2p::Thread::NORMAL + 0, Thread1, NULL, "Thread1");

  r2p::Thread::set_priority(r2p::Thread::NORMAL);
  for (;;) {
    palTogglePad(LED_GPIO, LED1);
    r2p::Thread::sleep(r2p::Time::ms(500));
  }
  return CH_SUCCESS;
}
}
