
#include "DebugSubscriber.hpp"
#include "DebugTransport.hpp"
#include <r2p/Topic.hpp>

namespace r2p {


size_t DebugSubscriber::get_queue_length() const {

  return tmsgp_queue.get_length();
}


size_t DebugSubscriber::get_queue_count_unsafe() const {

  return tmsgp_queue.get_count_unsafe();
}


bool DebugSubscriber::notify(BaseMessage &msg, const Time &timestamp) {

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
  return false;
}


bool DebugSubscriber::fetch(BaseMessage *&msgp, Time &timestamp) {

  TimestampedMsgPtrQueue::Entry entry;
  if (tmsgp_queue.fetch_unsafe(entry)) { // NOTE: This is unsafe!
    msgp = entry.msgp;
    timestamp = entry.timestamp;
    return true;
  }
  return false;
}


DebugSubscriber::DebugSubscriber(DebugTransport &transport,
                                 TimestampedMsgPtrQueue::Entry queue_buf[],
                                 size_t queue_length)
:
  RemoteSubscriber(transport),
  tmsgp_queue(queue_buf, queue_length),
  by_transport_notify(*this)
{}


DebugSubscriber::~DebugSubscriber() {}


} // namespace r2p
