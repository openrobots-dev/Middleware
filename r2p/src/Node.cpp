#include <r2p/Node.hpp>
#include <r2p/Middleware.hpp>
#include <r2p/Thread.hpp>
#include <r2p/MgmtMsg.hpp>
#include <r2p/Publisher.hpp>
#include <r2p/Subscriber.hpp>

namespace r2p {


bool Node::advertise(LocalPublisher &pub, const char *namep,
                     const Time &publish_timeout, size_t msg_size) {

  if (Middleware::instance.advertise(pub, namep, publish_timeout, msg_size)) {
    publishers.link(pub.by_node);
    return true;
  }
  return false;
}


bool Node::subscribe(LocalSubscriber &sub, const char *namep,
                     Message msgpool_buf[], size_t msg_size) {

  if (Middleware::instance.subscribe(sub, namep, msgpool_buf,
                                     sub.get_queue_length(), msg_size)) {
    sub.nodep = this;
    SysLock::acquire();
    int index = subscribers.get_count_unsafe();
    subscribers.link_unsafe(sub.by_node);
    SysLock::release();
    R2P_ASSERT(index >= 0);
    R2P_ASSERT(index <= static_cast<int>(SpinEvent::MAX_INDEX));
    sub.event_index = static_cast<uint_least8_t>(index);
    return true;
  }
  return false;
}


void Node::publish_publishers(Publisher<MgmtMsg> &info_pub) {

  MgmtMsg *msgp;

  for (StaticList<LocalPublisher>::Iterator i = publishers.begin();
       i != publishers.end(); ++i) {
    while (!info_pub.alloc(msgp)) {
      Thread::yield();
    }

    msgp->type = MgmtMsg::INFO_ADVERTISEMENT;
    ::strncpy(msgp->path.module, Middleware::instance.get_module_name(),
              NamingTraits<Middleware>::MAX_LENGTH);
    ::strncpy(msgp->path.node, this->get_name(),
              NamingTraits<Node>::MAX_LENGTH);
    ::strncpy(msgp->path.topic, i->get_topic()->get_name(),
              NamingTraits<Topic>::MAX_LENGTH);

    while (!info_pub.publish_remotely(*msgp)) {
      Thread::yield();
    }
  }
}


void Node::publish_subscribers(Publisher<MgmtMsg> &info_pub) {

  MgmtMsg *msgp;

  for (StaticList<LocalSubscriber>::Iterator i = subscribers.begin();
       i != subscribers.end(); ++i) {
    while (!info_pub.alloc(msgp)) {
      Thread::yield();
    }

    msgp->type = MgmtMsg::INFO_SUBSCRIPTION;
    ::strncpy(msgp->path.module, Middleware::instance.get_module_name(),
              NamingTraits<Middleware>::MAX_LENGTH);
    ::strncpy(msgp->path.node, this->get_name(),
              NamingTraits<Node>::MAX_LENGTH);
    ::strncpy(msgp->path.topic, i->get_topic()->get_name(),
              NamingTraits<Topic>::MAX_LENGTH);

    while (!info_pub.publish_remotely(*msgp)) {
      Thread::yield();
    }
  }
}


bool Node::spin(const Time &timeout) {

  SpinEvent::Mask mask;
  mask = event.wait(timeout);
  if (mask == 0) return false;

  Time dummy_timestamp;
  SpinEvent::Mask bit = 1;
  for (StaticList<LocalSubscriber>::Iterator i = subscribers.begin();
       i != subscribers.end() && mask != 0; bit <<= 1, ++i) {
    if ((mask & bit) != 0) {
      mask &= ~bit;
      const LocalSubscriber::Callback *callback = i->get_callback();
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


Node::Node(const char *namep)
:
  namep(namep),
  by_middleware(*this)
{
  R2P_ASSERT(is_identifier(namep));
  R2P_ASSERT(::strlen(namep) <= NamingTraits<Node>::MAX_LENGTH);

  Middleware::instance.add(*this);
}


}; // namespace r2p
