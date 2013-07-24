
#include <r2p/BaseTransport.hpp>
#include <r2p/BaseMessage.hpp>
#include <r2p/Middleware.hpp>
#include <r2p/RemotePublisher.hpp>
#include <r2p/RemoteSubscriber.hpp>
#include <r2p/TimestampedMsgPtrQueue.hpp>

namespace r2p {


bool BaseTransport::notify_advertisement(Topic &topic) {

  if (!touch_publisher(topic)) return false;
  if (!send_advertisement(topic)) return false;
  return true;
}


bool BaseTransport::notify_subscription(Topic &topic, size_t queue_length) {

  if (!touch_subscriber(topic, queue_length)) return false;
  if (!send_subscription(topic, queue_length)) return false;
  return true;
}


bool BaseTransport::notify_stop() {

  return send_stop();
}


bool BaseTransport::notify_reboot() {

  return send_reboot();
}



bool BaseTransport::touch_publisher(Topic &topic) {

  // Check if the remote publisher already exists
  const StaticList<RemotePublisher>::Link *linkp;
  linkp = publishers.find_first(BasePublisher::has_topic, topic.get_name());
  if (linkp == NULL) {
    // Create a new remote publisher
    RemotePublisher *pubp = create_publisher();
    if (pubp == NULL) return false;
    pubp->notify_advertised(topic);
    topic.advertise(*pubp, Time::INFINITE);
    publishers.link(pubp->by_transport);
  }
  return true;
}


bool BaseTransport::touch_subscriber(Topic &topic, size_t queue_length) {

  // Check if the remote subscriber already exists
  const StaticList<RemoteSubscriber>::Link *linkp;
  linkp = subscribers.find_first(BaseSubscriber::has_topic, topic.get_name());
  if (linkp != NULL) return true;

  // Create a new remote subscriber
  BaseMessage *msgpool_bufp = NULL;
  TimestampedMsgPtrQueue::Entry *queue_bufp = NULL;
  RemoteSubscriber *subp = NULL;

  msgpool_bufp = reinterpret_cast<BaseMessage *>(
    new uint8_t[topic.get_size() * queue_length]
  );
  if (msgpool_bufp != NULL) {
    queue_bufp = new TimestampedMsgPtrQueue::Entry[queue_length];
    if (queue_bufp != NULL) {
      subp = create_subscriber(*this, queue_bufp, queue_length);
      if (subp != NULL) {
        subp->notify_subscribed(topic);
        topic.extend_pool(msgpool_bufp, queue_length);
        topic.subscribe(*subp);
        subscribers.link(subp->by_transport);
        return true;
      }
    }
  }
  if (msgpool_bufp == NULL || queue_bufp == NULL || subp == NULL) {
    delete subp;
    delete [] queue_bufp;
    delete [] msgpool_bufp;
  }
  return false;
}


bool BaseTransport::advertise(const char *namep) {

  // Process only if there are local subscribers
  Topic *topicp = Middleware::instance.find_topic(namep);
  if (topicp == NULL || !topicp->has_local_subscribers()) return true;
  return touch_publisher(*topicp);
}


bool BaseTransport::subscribe(const char *namep, size_t queue_length) {

  // Process only if there are local publishers
  Topic *topicp = Middleware::instance.find_topic(namep);
  if (topicp == NULL || !topicp->has_local_publishers()) return true;
  return touch_subscriber(*topicp, queue_length);
}


bool BaseTransport::advertise(RemotePublisher &pub, const char *namep,
                              const Time &publish_timeout, size_t type_size) {

  if (Middleware::instance.advertise(pub, namep, publish_timeout, type_size)) {
    publishers.link(pub.by_transport);
    return true;
  }
  return false;
}


bool BaseTransport::subscribe(RemoteSubscriber &sub, const char *namep,
                              BaseMessage msgpool_buf[], size_t msgpool_buflen,
                              size_t type_size) {

  if (Middleware::instance.subscribe(sub, namep, msgpool_buf, msgpool_buflen,
                                     type_size)) {
    subscribers.link(sub.by_transport);
    return true;
  }
  return false;
}


BaseTransport::BaseTransport(const char *namep)
:
  Named(namep),
  by_middleware(*this)
{}


BaseTransport::~BaseTransport() {}


} // namespace r2p
