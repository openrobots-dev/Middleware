#pragma once

#include <r2p/common.hpp>
#include <r2p/StaticList.hpp>
#include <r2p/TimestampedMsgPtrQueue.hpp>
#include <r2p/Mutex.hpp>
#include <r2p/NamingTraits.hpp>

namespace r2p {

class Message;
class Time;
class Topic;
class RemotePublisher;
class RemoteSubscriber;


class Transport : private Uncopyable {
  friend class Middleware;

protected:
  const char *namep;
  StaticList<RemotePublisher> publishers;
  StaticList<RemoteSubscriber> subscribers;
  Mutex publishers_lock;
  Mutex subscribers_lock;

private:
  mutable StaticList<Transport>::Link by_middleware;

public:
  const char *get_name() const;

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
  Transport(const char *namep);
  virtual ~Transport() = 0;

public:
  static bool has_name(const Transport &transport, const char *namep);
};


inline
const char *Transport::get_name() const {

  return namep;
}


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


inline
bool Transport::has_name(const Transport &transport, const char *namep) {

  return namep != NULL &&
         0 == strncmp(transport.get_name(), namep,
                      NamingTraits<Transport>::MAX_LENGTH);
}


} // namespace r2p
