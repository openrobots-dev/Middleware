
#include "RTCANSubscriber.hpp"
#include "RTCANTransport.hpp"
#include <r2p/Topic.hpp>

namespace r2p {

bool RTCANSubscriber::notify(BaseMessage &msg, const Time &timestamp) {
	(void) msg;
	(void) timestamp;
/*
  TimestampedMsgPtrQueue::Entry entry(&msg, timestamp);
  SysLock::acquire();
  if (tmsgp_queue.post_unsafe(entry)) {
    if (tmsgp_queue.get_count_unsafe() == 1) {
      static_cast<DebugTransport *>(get_transport())
        ->notify_first_sub_unsafe(*this);
    }
    SysLock::release();
    return true;
  }
  SysLock::release();
  */
  return false;
}


bool RTCANSubscriber::fetch(BaseMessage *&msgp, Time &timestamp) {
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


RTCANSubscriber::RTCANSubscriber(RTCANTransport &transport,
                                 size_t queue_length)
:
  RemoteSubscriber(transport),
  queue_free(queue_length)
{}


RTCANSubscriber::~RTCANSubscriber() {}


} // namespace r2p
