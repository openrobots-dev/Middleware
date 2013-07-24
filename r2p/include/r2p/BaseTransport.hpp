
#ifndef __R2P__BASETRANSPORT_HPP__
#define __R2P__BASETRANSPORT_HPP__

#include <r2p/common.hpp>
#include <r2p/StaticList.hpp>
#include <r2p/TimestampedMsgPtrQueue.hpp>

namespace r2p {

class BaseMessage;
class Time;
class Topic;
class RemotePublisher;
class RemoteSubscriber;


class BaseTransport : public Named, private Uncopyable {
  friend class Middleware;

protected:
  StaticList<RemotePublisher> publishers;
  StaticList<RemoteSubscriber> subscribers;

private:
  StaticList<BaseTransport>::Link by_middleware;

public:
  bool notify_advertisement(Topic &topic);
  bool notify_subscription(Topic &topic, size_t queue_length);
  bool notify_stop();
  bool notify_reboot();

protected:
  bool touch_publisher(Topic &topic);
  bool touch_subscriber(Topic &topic, size_t queue_length);

  bool advertise(const char *namep);
  bool subscribe(const char *namep, size_t queue_length);

  bool advertise(RemotePublisher &pub, const char *namep,
                 const Time &publish_timeout, size_t type_size);
  bool subscribe(RemoteSubscriber &sub, const char *namep,
                 BaseMessage msgpool_buf[], size_t msgpool_buflen,
                 size_t type_size);

  virtual bool send_advertisement(const Topic &topic) = 0;
  virtual bool send_subscription(const Topic &topic, size_t queue_length) = 0;
  virtual bool send_stop() = 0;
  virtual bool send_reboot() = 0;

  virtual RemotePublisher *create_publisher() = 0;
  virtual RemoteSubscriber *create_subscriber(
    BaseTransport &transport,
    TimestampedMsgPtrQueue::Entry queue_buf[],
    size_t queue_length
  ) = 0;

protected:
  BaseTransport(const char *namep);
  virtual ~BaseTransport() = 0;
};


} // namespace r2p
#endif // __R2P__BASETRANSPORT_HPP__
