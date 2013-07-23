
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
      transportp->notify_first_sub_unsafe(*this);
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


DebugSubscriber::DebugSubscriber(TimestampedMsgPtrQueue::Entry queue_buf[],
                                 size_t queue_length,
                                 DebugTransport &transport)
:
  transportp(&transport),
  tmsgp_queue(queue_buf, queue_length),
  by_transport(*this),
  by_transport_notify(*this)
{}


bool DebugSubscriber::has_topic(const DebugSubscriber &sub,
                                const char *namep) {

  return Topic::has_name(*sub.get_topic(), namep);
}


} // namespace r2p
