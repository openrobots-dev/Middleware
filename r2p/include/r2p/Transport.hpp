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
  bool notify_advertisement(Topic &topic);
  bool notify_subscription(Topic &topic);
  bool notify_stop();
  bool notify_reboot();

protected:
  bool touch_publisher(Topic &topic);
  bool touch_subscriber(Topic &topic, size_t queue_length);

  bool advertise(Topic &topic);
  bool subscribe(Topic &topic, size_t queue_length);

  bool advertise(RemotePublisher &pub, const char *namep,
                 const Time &publish_timeout, size_t type_size);
  bool subscribe(RemoteSubscriber &sub, const char *namep,
                 Message msgpool_buf[], size_t msgpool_buflen,
                 size_t type_size);

  virtual bool send_advertisement(const Topic &topic) = 0;
  virtual bool send_subscription(const Topic &topic, size_t queue_length) = 0;
  virtual bool send_stop() = 0;
  virtual bool send_reboot() = 0;

  virtual RemotePublisher *create_publisher(Topic &topic) const = 0;
  virtual RemoteSubscriber *create_subscriber( // FIXME: Aggiungere topic pure qua?
    Transport &transport,
    TimestampedMsgPtrQueue::Entry queue_buf[], // TODO: Make as generic <const void *> argument
    size_t queue_length
  ) const = 0;

protected:
  Transport();
  virtual ~Transport() = 0;

public:
  static bool has_name(const Transport &transport);
};


} // namespace r2p
