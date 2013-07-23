
#ifndef __R2P__DEBUGSUBSCRIBER_HPP__
#define __R2P__DEBUGSUBSCRIBER_HPP__

#include <r2p/common.hpp>
#include <r2p/BaseSubscriber.hpp>
#include <r2p/TimestampedMsgPtrQueue.hpp>
#include <r2p/StaticList.hpp>
#include <r2p/BaseSubscriberQueue.hpp>
#include <ch.h>

namespace r2p {

class BaseMessage;
class DebugTransport;


class DebugSubscriber : public BaseSubscriber {
protected:
  DebugTransport *transportp;
  TimestampedMsgPtrQueue tmsgp_queue;

public:
  mutable class ListEntryByDebugTransport : private r2p::Uncopyable {
    friend class DebugSubscriber; friend class DebugTransport;
    StaticList<DebugSubscriber>::Link entry;
    ListEntryByDebugTransport(DebugSubscriber &sub) : entry(sub) {}
  } by_transport;

  mutable class ListEntryByDebugTransportNotify : private r2p::Uncopyable {
    friend class DebugSubscriber; friend class DebugTransport;
    BaseSubscriberQueue::Link entry;
    ListEntryByDebugTransportNotify(DebugSubscriber &sub) : entry(sub) {}
  } by_transport_notify;

public:
  size_t get_queue_length() const;
  size_t get_queue_count_unsafe() const;
  bool fetch(BaseMessage *&msgp, Time &timestamp);
  bool notify(BaseMessage &msg, const Time &timestamp);

public:
  DebugSubscriber(TimestampedMsgPtrQueue::Entry queue_buf[],
                  size_t queue_length, DebugTransport &transport);

public:
  static bool has_topic(const DebugSubscriber &sub, const char *namep);
};


} // namespace r2p
#endif // __R2P__DEBUGSUBSCRIBER_HPP__
