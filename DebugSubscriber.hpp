
#ifndef __R2P__DEBUGSUBSCRIBER_HPP__
#define __R2P__DEBUGSUBSCRIBER_HPP__

#include <r2p/common.hpp>
#include <r2p/RemoteSubscriber.hpp>
#include <r2p/TimestampedMsgPtrQueue.hpp>
#include <r2p/StaticList.hpp>
#include <r2p/BaseSubscriberQueue.hpp>
#include <ch.h>

namespace r2p {

class BaseMessage;
class DebugTransport;


class DebugSubscriber : public RemoteSubscriber {
  friend class DebugTransport;

private:
  TimestampedMsgPtrQueue tmsgp_queue;

private:
  BaseSubscriberQueue::Link by_transport_notify;

public:
  size_t get_queue_length() const;
  size_t get_queue_count_unsafe() const;
  bool fetch(BaseMessage *&msgp, Time &timestamp);
  bool notify(BaseMessage &msg, const Time &timestamp);

public:
  DebugSubscriber(DebugTransport &transport,
                  TimestampedMsgPtrQueue::Entry queue_buf[],
                  size_t queue_length);
  virtual ~DebugSubscriber();
};


} // namespace r2p
#endif // __R2P__DEBUGSUBSCRIBER_HPP__
