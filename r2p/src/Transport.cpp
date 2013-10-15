
#include <r2p/Transport.hpp>
#include <r2p/Message.hpp>
#include <r2p/Middleware.hpp>
#include <r2p/RemotePublisher.hpp>
#include <r2p/RemoteSubscriber.hpp>
#include <r2p/TimestampedMsgPtrQueue.hpp>
#include <r2p/ScopedLock.hpp>
#include "r2p/transport/RTCANPublisher.hpp"

namespace r2p {


bool Transport::touch_publisher(Topic &topic, uint8_t * raw_params) {

  ScopedLock<Mutex> lock(publishers_lock);

  // Check if the remote publisher already exists
  RemotePublisher *pubp;
  pubp = publishers.find_first(BasePublisher::has_topic, topic.get_name());
  if (pubp != NULL) return true;

  // Create a new remote publisher
  pubp = create_publisher(topic, raw_params);
  if (pubp == NULL) return false;

  pubp->notify_advertised(topic);
  topic.advertise(*pubp, Time::INFINITE);
  publishers.link(pubp->by_transport);

  return true;
}


bool Transport::touch_subscriber(Topic &topic, size_t queue_length, uint8_t * raw_params) {

  ScopedLock<Mutex> lock(subscribers_lock);

  // Check if the remote subscriber already exists
  RemoteSubscriber *subp;
  subp = subscribers.find_first(BaseSubscriber::has_topic, topic.get_name());
  if (subp != NULL) return true;

  // Create a new remote subscriber
  Message *msgpool_bufp = NULL;
  TimestampedMsgPtrQueue::Entry *queue_bufp = NULL;

  msgpool_bufp = reinterpret_cast<Message *>(
    new uint8_t[(topic.get_size() + sizeof(Message)) * queue_length]
  );
  if (msgpool_bufp != NULL) {
    queue_bufp = new TimestampedMsgPtrQueue::Entry[queue_length];
    if (queue_bufp != NULL) {
      subp = create_subscriber(topic, queue_bufp, queue_length, raw_params);
      if (subp != NULL) {
        topic.extend_pool(msgpool_bufp, queue_length);
        subp->notify_subscribed(topic);
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


bool Transport::advertise_cb(Topic &topic, uint8_t * raw_params) {

  // Process only if there are local subscribers
  SysLock::acquire();
  if (topic.has_local_subscribers()) {
    SysLock::release();
    return touch_publisher(topic, raw_params);
  } else {
    SysLock::release();
    return true;
  }
}


bool Transport::subscribe_cb(Topic &topic, size_t queue_length, uint8_t * raw_params) {

  // Process only if there are local publishers
  SysLock::acquire();
  if (topic.has_local_publishers()) { // FIXME: isn't this already checked? and probably also for advertise_cb() [MARTINO]
    SysLock::release();
    return touch_subscriber(topic, queue_length, raw_params);
  } else {
    SysLock::release();
    return true;
  }
}


bool Transport::advertise(RemotePublisher &pub, const char *namep,
                          const Time &publish_timeout, size_t type_size) {

  publishers_lock.acquire();
  if (Middleware::instance.advertise(pub, namep, publish_timeout, type_size)) {
    publishers.link(pub.by_transport);
    publishers_lock.release();
    return true;
  } else {
    publishers_lock.release();
    return false;
  }
}


bool Transport::subscribe(RemoteSubscriber &sub, const char *namep,
                          Message msgpool_buf[], size_t msgpool_buflen,
                          size_t type_size) {

  subscribers_lock.acquire();
  if (Middleware::instance.subscribe(sub, namep, msgpool_buf, msgpool_buflen,
                                     type_size)) {
    subscribers.link(sub.by_transport);
    subscribers_lock.release();
    return true;
  } else {
    subscribers_lock.release();
    return false;
  }
}


Transport::Transport(const char *namep)
:
  namep(namep),
  by_middleware(*this)
{
  R2P_ASSERT(is_identifier(namep, NamingTraits<Transport>::MAX_LENGTH));
}


Transport::~Transport() {}


} // namespace r2p
