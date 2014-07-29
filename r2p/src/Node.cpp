#include <r2p/Node.hpp>
#include <r2p/Middleware.hpp>
#include <r2p/Thread.hpp>
#include <r2p/MgmtMsg.hpp>
#include <r2p/Publisher.hpp>
#include <r2p/Subscriber.hpp>

namespace r2p {


bool Node::advertise(LocalPublisher &pub, const char *namep,
                     const Time &publish_timeout, size_t msg_size) {

  // already advertised
  if (pub.get_topic() != NULL) return false;

  if (Middleware::instance.advertise(pub, namep, publish_timeout, msg_size)) {
    publishers.link(pub.by_node);
    return true;
  }
  return false;
}


bool Node::subscribe(LocalSubscriber &sub, const char *namep,
                     Message msgpool_buf[], size_t msg_size) {

  // already subscribed
  if (sub.get_topic() != NULL) return false;

  sub.nodep = this;
  int index = subscribers.count();
  subscribers.link(sub.by_node);
  R2P_ASSERT(index >= 0);
  R2P_ASSERT(index <= static_cast<int>(SpinEvent::MAX_INDEX));
  sub.event_index = static_cast<uint_least8_t>(index);

  if (!Middleware::instance.subscribe(sub, namep, msgpool_buf,
                                     sub.get_queue_length(), msg_size)) {
    subscribers.unlink(sub.by_node);
    return false;
  }

  return true;

}


bool Node::spin(const Time &timeout) {

  SpinEvent::Mask mask;
  mask = event.wait(timeout);

  if (mask == 0) {
	  return false;
  }

  Time dummy_timestamp;
  SpinEvent::Mask bit = 1;
  for (StaticList<LocalSubscriber>::Iterator i = subscribers.begin();
       i != subscribers.end() && mask != 0; bit <<= 1, ++i) {
    if ((mask & bit) != 0) {
      mask &= ~bit;
      const LocalSubscriber::Callback callback = i->get_callback();
      if (callback != NULL) {
        Message *msgp;
        while (i->fetch(msgp, dummy_timestamp)) {
          (*callback)(*msgp);
          i->release(*msgp);
        }
      }
    }
  }
  return true;
}


Node::Node(const char *namep, bool enabled)
:
  namep(namep),
  event(enabled ? &Thread::self() : NULL),
  by_middleware(*this)
{
  R2P_ASSERT(is_identifier(namep, NamingTraits<Node>::MAX_LENGTH));

  Middleware::instance.add(*this);
}


Node::~Node() {

  Middleware::instance.confirm_stop(*this);
}


}; // namespace r2p
