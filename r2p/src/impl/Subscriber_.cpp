
#include <r2p/impl/Subscriber_.hpp>
#include <r2p/Node.hpp>

namespace r2p {


Subscriber_::Callback Subscriber_::get_callback() const {

  return callback;
}


size_t Subscriber_::get_queue_length() const {

  return msgp_queue.get_length();
}


bool Subscriber_::fetch(BaseMessage *&msgp, Time &timestamp) {

  if (msgp_queue.fetch(msgp)) {
    timestamp = Time::now();
    return true;
  }
  return false;
}


bool Subscriber_::notify(BaseMessage &msg, const Time &timestamp) {

  (void)timestamp;
  if (msgp_queue.post(&msg)) {
    nodep->notify(event_index);
    return true;
  }
  return false;
}


Subscriber_::Subscriber_(BaseMessage *queue_buf[], size_t queue_length,
                                 Callback callback) :

  BaseSubscriber(),
  nodep(NULL),
  callback(callback),
  msgp_queue(queue_buf, queue_length),
  event_index(~0),
  by_node(*this)
{
  R2P_ASSERT(queue_buf != NULL);
  R2P_ASSERT(queue_length > 0);
}


Subscriber_::~Subscriber_() {}


} // namespace r2p
