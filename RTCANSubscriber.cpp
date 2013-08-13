
#include "RTCANSubscriber.hpp"
#include "RTCANTransport.hpp"
#include <r2p/Topic.hpp>

namespace r2p {

bool RTCANSubscriber::fetch_unsafe(Message *&msgp, Time &timestamp) {
	(void) msgp;
	(void) timestamp;
/*
  TimestampedMsgPtrQueue::Entry entry;
  if (tmsgp_queue.fetch_unsafe(entry)) { // NOTE: This is unsafe!
    msgp = entry.msgp;
    timestamp = entry.timestamp;
    return true;
  }
  */
  return false;
}

#include "hal.h"
#include "board.h"


bool RTCANSubscriber::notify_unsafe(Message &msg, const Time &timestamp) {
	(void) msg;
	(void) timestamp;

	if (queue_free > 0) {
		queue_free--;
		palTogglePad(LED_GPIO, LED3);
		release_unsafe(msg);
		return true;
	}

	return false;
}


size_t RTCANSubscriber::get_queue_length() const {
	return 0;
}

RTCANSubscriber::RTCANSubscriber(RTCANTransport &transport,
		                         TimestampedMsgPtrQueue::Entry queue_buf[],
                                 size_t queue_length)
:
  RemoteSubscriber(transport),
  queue_free(queue_length)
{}


RTCANSubscriber::~RTCANSubscriber() {}


} // namespace r2p
