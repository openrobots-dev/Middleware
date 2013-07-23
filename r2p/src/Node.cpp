
#include <r2p/Node.hpp>
#include <r2p/Middleware.hpp>
#include <r2p/Thread.hpp>

namespace r2p {


bool Node::advertise(Publisher_ &pub, const char *namep,
                     const Time &publish_timeout, size_t msg_size) {

  R2P_ASSERT(namep != NULL);
  R2P_ASSERT(namep[0] != 0);

  if (Middleware::instance.advertise(pub, namep, publish_timeout, msg_size)) {
    publishers.link(pub.by_node.entry);
    return true;
  }
  return false;
}


bool Node::subscribe(Subscriber_ &sub, const char *namep,
                     BaseMessage msgpool_buf[], size_t msg_size) {

  R2P_ASSERT(namep != NULL);
  R2P_ASSERT(namep[0] != 0);

  if (Middleware::instance.subscribe_local(sub, namep, msgpool_buf,
                                           sub.get_queue_length(), msg_size)) {
    sub.nodep = this;
    SysLock::acquire();
    int index = subscribers.get_count_unsafe();
    subscribers.link_unsafe(sub.by_node.entry);
    SysLock::release();
    R2P_ASSERT(index >= 0);
    R2P_ASSERT(index < static_cast<int>(sizeof(eventmask_t) * 8));
    sub.event_index = static_cast<unsigned>(index);
    return true;
  }
  return false;
}


void Node::publish_publishers(Publisher<InfoMsg> &info_pub) {

  InfoMsg *msgp;

  for (StaticList<Publisher_>::Iterator i = publishers.begin();
       i != publishers.end(); ++i) {
    while (!info_pub.alloc(msgp)) {
      Thread::yield();
    }

    msgp->type = InfoMsg::ADVERTISEMENT;
    ::strncpy(msgp->path.module, Middleware::instance.get_name(),
              NamingTraits<Middleware>::MAX_LENGTH);
    ::strncpy(msgp->path.node, this->get_name(),
              NamingTraits<Node>::MAX_LENGTH);
    ::strncpy(msgp->path.topic, i->datap->get_topic()->get_name(),
              NamingTraits<Topic>::MAX_LENGTH);

    while (!info_pub.publish_remotely(*msgp)) {
      Thread::yield();
    }
  }
}


void Node::publish_subscribers(Publisher<InfoMsg> &info_pub) {

  InfoMsg *msgp;

  for (StaticList<Subscriber_>::Iterator i = subscribers.begin();
       i != subscribers.end(); ++i) {
    while (!info_pub.alloc(msgp)) {
      Thread::yield();
    }

    msgp->type = InfoMsg::SUBSCRIPTION;
    ::strncpy(msgp->path.module, Middleware::instance.get_name(),
              NamingTraits<Middleware>::MAX_LENGTH);
    ::strncpy(msgp->path.node, this->get_name(),
              NamingTraits<Node>::MAX_LENGTH);
    ::strncpy(msgp->path.topic, i->datap->get_topic()->get_name(),
              NamingTraits<Topic>::MAX_LENGTH);

    while (!info_pub.publish_remotely(*msgp)) {
      Thread::yield();
    }
  }
}


bool Node::spin(const Time &timeout) {

  SpinEvent::Mask mask, bit = 1;
  mask = event.wait(timeout);
  if (mask == 0) return false;

  BaseMessage *msgp;
  Time dummy_timestamp;
  for (StaticList<Subscriber_>::Iterator i = subscribers.begin();
       i != subscribers.end() && mask != 0; bit <<= 1, ++i) {
    if ((mask & bit) != 0) {
      mask &= ~bit;
      Subscriber_ &sub = *i->datap;
      Subscriber_::Callback callback = sub.get_callback();
      if (callback != NULL) {
        while (sub.fetch(msgp, dummy_timestamp)) {
          callback(*msgp);
          sub.release(*msgp);
        }
      }
    }
  }
  return true;
}


Node::Node(const char *namep)
:
  Named(namep),
  by_middleware(*this)
{
  R2P_ASSERT(namep != NULL);
  R2P_ASSERT(namep[0] != 0);
}


}; // namespace r2p
