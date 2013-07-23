
#include "ch.h"
#include "hal.h"

#include <r2p/common.hpp>
#include <r2p/Middleware.hpp>
#include <r2p/Node.hpp>
#include <r2p/Topic.hpp>
#include <r2p/Publisher.hpp>
#include <r2p/Subscriber.hpp>

#include <cstdio>
#include <cstdlib>
#include <cstring>


R2P_MSGDEF_BEGIN(MsgUint32)
  uint32_t value;
R2P_MSGDEF_END()


R2P_MSGDEF_BEGIN(MsgFloat)
  float value;
R2P_MSGDEF_END()


r2p::Middleware r2p::Middleware::instance("PosixBoard");

static WORKING_AREA(wa1, 8192);
static WORKING_AREA(wa2, 8192);

static r2p::Mutex print_lock;


msg_t Thread1(void *) {

  static MsgUint32 sub_msgbuf[5], *sub_queue[5];
  static r2p::Subscriber<MsgUint32> sub_uint32(5, sub_queue, sub_msgbuf, NULL);
  static r2p::Publisher<MsgFloat> pub_float;
  static r2p::Node node("Node1");

  r2p::Middleware::instance.add(node);
  node.subscribe(sub_uint32, "topic_uint32");
  node.advertise(pub_float, "topic_float");

  for (;;) {
    MsgUint32 *uint32_msgp;
    while (sub_uint32.fetch(uint32_msgp)) {
      print_lock.acquire();
      printf("sub_uint32: %d\n", uint32_msgp->value);
      print_lock.release();
      sub_uint32.release(*uint32_msgp);
    }

    chThdSleepMilliseconds(250);
    MsgFloat *float_msgp;
    if (pub_float.alloc(float_msgp)) {
      float_msgp->value = 123.0f;
      pub_float.publish(*float_msgp);
    }
  }
  return CH_SUCCESS;
}


bool print_floatmsg(const MsgFloat &msg) {

  print_lock.acquire();
  printf("sub_float: %.3f\n", msg.value);
  print_lock.release();
  return true;
}


msg_t Thread2(void *) {

  static MsgFloat sub_msgbuf[5], *sub_queue[5];
  static r2p::Subscriber<MsgFloat> sub_float(5, sub_queue, sub_msgbuf,
                                             print_floatmsg);
  static r2p::Publisher<MsgFloat> pub_float;
  static r2p::Node node("Node2");

  r2p::Middleware::instance.add(node);
  node.subscribe(sub_float, "topic_float");

  for (;;) {
    node.spin(r2p::Time::INFINITE);
  }
  return CH_SUCCESS;
}


extern "C" {
int main(void) {

  halInit();
  chSysInit();

  chThdCreateStatic(wa1, sizeof(wa1), NORMALPRIO, Thread1, NULL);
  chThdCreateStatic(wa2, sizeof(wa2), NORMALPRIO, Thread2, NULL);

  for (;;) {
    chThdSleepSeconds(1);
  }
  return CH_SUCCESS;
}
}
