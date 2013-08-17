
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
<<<<<<< HEAD
  R2P_ASSERT(msg.refcount < 4); // XXX
=======
  SysLock::acquire();
>>>>>>> 7cecf40226ac6a53f09682a1c864ff5d47ccddf6
  if (msgp_queue.post_unsafe(&msg)) {
    nodep->notify_unsafe(event_index);
    SysLock::release();
    return true;
  } else {
    SysLock::release();
    return false;
  }
<<<<<<< HEAD
  return false;
}


bool LocalSubscriber::fetch(Message *&msgp, Time &timestamp) {

  if (msgp_queue.fetch(msgp)) {
    R2P_ASSERT(msgp->refcount < 4); // XXX

    timestamp = Time::now();
    return true;
  }
  return false;
}


bool LocalSubscriber::notify(Message &msg, const Time &timestamp) {

  (void)timestamp;
  if (msgp_queue.post(&msg)) {
    nodep->notify(event_index);
    return true;
  }
  return false;
=======
>>>>>>> 7cecf40226ac6a53f09682a1c864ff5d47ccddf6
}


LocalSubscriber::LocalSubscriber(Message *queue_buf[], size_t queue_length,
                                 const Callback *callbackp)
:
  BaseSubscriber(),
  nodep(NULL),
  callbackp(callbackp),
  msgp_queue(queue_buf, queue_length),
  event_index(~0),
  by_node(*this),
  by_topic(*this)
{
  R2P_ASSERT(queue_buf != NULL);
  R2P_ASSERT(queue_length > 0);
}


LocalSubscriber::~LocalSubscriber() {}


} // namespace r2p
