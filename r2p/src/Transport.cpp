
#include <r2p/Transport.hpp>
#include <r2p/Message.hpp>
#include <r2p/Middleware.hpp>
#include <r2p/RemotePublisher.hpp>
#include <r2p/RemoteSubscriber.hpp>
#include <r2p/TimestampedMsgPtrQueue.hpp>

namespace r2p {


bool Transport::notify_advertisement(Topic &topic) {

  if (!send_advertisement(topic)) return false;
  return true;
}


bool Transport::notify_subscription(Topic &topic) {

  // FIXME: Get the queue length by the topic itself
  if (!send_subscription(topic, topic.get_max_queue_length())) return false;
  return true;
}


bool Transport::notify_stop() {

  return send_stop();
}


bool Transport::notify_reboot() {

  return send_reboot();
}


bool Transport::touch_publisher(Topic &topic) {

  Mutex::ScopedLock lock(publishers_lock);

  // Check if the remote publisher already exists
  register const StaticList<RemotePublisher>::Link *linkp;
  linkp = publishers.find_first(BasePublisher::has_topic, topic.get_name());
  if (linkp != NULL) return true;

  // Create a new remote publisher
  RemotePublisher *pubp = create_publisher();
  if (pubp == NULL) return false;

  pubp->notify_advertised(topic);
  topic.advertise(*pubp, Time::INFINITE);
  publishers.link(pubp->by_transport);
  return true;
}


bool Transport::touch_subscriber(Topic &topic, size_t queue_length) {

  Mutex::ScopedLock lock(subscribers_lock);

  // Check if the remote subscriber already exists
  const StaticList<RemoteSubscriber>::Link *linkp;
  linkp = subscribers.find_first(BaseSubscriber::has_topic, topic.get_name());
  if (linkp != NULL) return true;

  // Create a new remote subscriber
  Message *msgpool_bufp = NULL;
  TimestampedMsgPtrQueue::Entry *queue_bufp = NULL;
  RemoteSubscriber *subp = NULL;

  msgpool_bufp = reinterpret_cast<Message *>(
    new uint8_t[topic.get_size() * queue_length]
  );
  if (msgpool_bufp != NULL) {
    queue_bufp = new TimestampedMsgPtrQueue::Entry[queue_length];
    if (queue_bufp != NULL) {
      subp = create_subscriber(*this, queue_bufp, queue_length);
      if (subp != NULL) {
        subp->notify_subscribed(topic);
        topic.extend_pool(msgpool_bufp, queue_length);
        topic.subscribe(*subp, queue_length);
        subscribers.link(subp->by_transport);
        return true;
      }
    }
  }
  if (msgpool_bufp == NULL || queue_bufp == NULL || subp == NULL) {
    delete [] queue_bufp;
    delete [] msgpool_bufp;
  }
  return false;
}


bool Transport::advertise(Topic &topic) {

  // Process only if there are local subscribers
  SysLock::acquire();
  if (topic.has_local_subscribers()) {
    SysLock::release();
    return touch_publisher(topic);
  } else {
    SysLock::release();
    return true;
  }
}


bool Transport::subscribe(Topic &topic, size_t queue_length) {

  // Process only if there are local publishers
  SysLock::acquire();
  if (topic.has_local_publishers()) {
    SysLock::release();
    return touch_subscriber(topic, queue_length);
  } else {
    SysLock::release();
    return true;
  }
}


bool Transport::advertise(RemotePublisher &pub, const char *namep,
                          const Time &publish_timeout, size_t type_size) {

  if (Middleware::instance.advertise(pub, namep, publish_timeout, type_size)) {
    publishers.link(pub.by_transport);
    return true;
  }
  return false;
}


bool Transport::subscribe(RemoteSubscriber &sub, const char *namep,
                          Message msgpool_buf[], size_t msgpool_buflen,
                          size_t type_size) {

  if (Middleware::instance.subscribe(sub, namep, msgpool_buf, msgpool_buflen,
                                     type_size)) {
    subscribers.link(sub.by_transport);
    return true;
  }
  return false;
}


Transport::Transport()
:
  by_middleware(*this)
{}


Transport::~Transport() {}


} // namespace r2p
