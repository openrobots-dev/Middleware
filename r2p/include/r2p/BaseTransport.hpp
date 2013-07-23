
#ifndef __R2P__BASETRANSPORT_HPP__
#define __R2P__BASETRANSPORT_HPP__

#include <r2p/common.hpp>
#include <r2p/StaticList.hpp>
#include <r2p/TimestampedMsgPtrQueue.hpp>

namespace r2p {

class BaseMessage;
class Time;
class Topic;
class BasePublisher;
class BaseSubscriber;


class BaseTransport : public Named, private r2p::Uncopyable {
public:
  mutable class ListEntryByMiddleware : private Uncopyable {
    friend class BaseTransport; friend class Middleware;
    StaticList<BaseTransport>::Link entry;
    ListEntryByMiddleware(BaseTransport &transport) : entry(transport) {}
  } by_middleware;

public:
  virtual void notify_advertise(Topic &topic) = 0;
  virtual void notify_subscribe(Topic &topic, size_t queue_length) = 0;
  virtual void notify_stop() = 0;
  virtual void notify_reboot() = 0;

protected:
  BaseTransport(const char *namep);
  virtual ~BaseTransport() = 0;
};


} // namespace r2p
#endif // __R2P__BASETRANSPORT_HPP__
