#include <r2p/LocalSubscriber.hpp>
#include <r2p/Node.hpp>

namespace r2p {


bool LocalSubscriber::fetch(Message *&msgp, Time &timestamp) {

  SysLock::acquire();
  if (msgp_queue.fetch_unsafe(msgp)) {
    SysLock::release();
    timestamp = Time::now();
    return true;
  } else {
    SysLock::release();
    return false;
  }
}


bool LocalSubscriber::notify(Message &msg, const Time &timestamp) {

  (void)timestamp;
  SysLock::acquire();
  if (msgp_queue.post_unsafe(&msg)) {
    nodep->notify_unsafe(event_index);
    SysLock::release();
    return true;
  } else {
    SysLock::release();
    return false;
  }
}


LocalSubscriber::LocalSubscriber(Message *queue_buf[], size_t queue_length,
                                 Callback callback)
:
  BaseSubscriber(),
  nodep(NULL),
  callback(callback),
  msgp_queue(queue_buf, queue_length),
  event_index(~0),
  by_node(*this),
  by_topic(*this)
{
  R2P_ASSERT(queue_buf != NULL);
  R2P_ASSERT(queue_length > 0);
}


LocalSubscriber::~LocalSubscriber() {}

}; // namespace r2p
