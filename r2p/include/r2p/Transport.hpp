#pragma once

#include <r2p/common.hpp>
#include <r2p/StaticList.hpp>
#include <r2p/TimestampedMsgPtrQueue.hpp>
#include <r2p/Mutex.hpp>

namespace r2p {

class Message;
class Time;
class Topic;
class RemotePublisher;
class RemoteSubscriber;


class Transport : private Uncopyable {
  friend class Middleware;

protected:
  StaticList<RemotePublisher> publishers;
  StaticList<RemoteSubscriber> subscribers;
  Mutex publishers_lock;
  Mutex subscribers_lock;

private:
  mutable StaticList<Transport>::Link by_middleware;

public:
  bool notify_advertisement(const Topic &topic);
  bool notify_subscription_request(const Topic &topic);
  bool notify_subscription_response(const Topic &topic);
  bool notify_stop();
  bool notify_reboot();

protected:
  bool touch_publisher(Topic &topic, uint8_t * raw_params = NULL);
  bool touch_subscriber(Topic &topic, size_t queue_length,
                        uint8_t *raw_params = NULL);

  bool advertise_cb(Topic &topic, uint8_t * raw_params = NULL);
  bool subscribe_cb(Topic &topic, size_t queue_length,
                    uint8_t *raw_params = NULL);

  bool advertise(RemotePublisher &pub, const char *namep,
                 const Time &publish_timeout, size_t type_size);
  bool subscribe(RemoteSubscriber &sub, const char *namep,
                 Message msgpool_buf[], size_t msgpool_buflen,
                 size_t type_size);

  virtual bool send_advertisement(const Topic &topic) = 0;
  virtual bool send_subscription_request(const Topic &topic) = 0;
  virtual bool send_subscription_response(const Topic &topic) = 0;
  virtual bool send_stop() = 0;
  virtual bool send_reboot() = 0;

  virtual RemotePublisher *create_publisher(Topic &topic,
                                            const uint8_t *raw_params = NULL)
                                            const = 0;
  virtual RemoteSubscriber *create_subscriber(
    Topic &topic,
    TimestampedMsgPtrQueue::Entry queue_buf[], // TODO: Make as generic <const void *> argument
    size_t queue_length,
    const uint8_t *raw_params = NULL
  ) const = 0;

protected:
  Transport();
  virtual ~Transport() = 0;
};


inline
bool Transport::notify_advertisement(const Topic &topic) {

  return send_advertisement(topic);
}


inline
bool Transport::notify_subscription_request(const Topic &topic) {

  return send_subscription_request(topic);
}


inline
bool Transport::notify_subscription_response(const Topic &topic) {

  return send_subscription_response(topic);
}


} // namespace r2p
