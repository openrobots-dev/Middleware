#pragma once

#include <r2p/common.hpp>
#include <r2p/RemoteSubscriber.hpp>
#include <r2p/TimestampedMsgPtrQueue.hpp>
#include <r2p/StaticList.hpp>
#include <r2p/BaseSubscriberQueue.hpp>
#include <ch.h>

namespace r2p {

class Message;
class DebugTransport;


class DebugSubscriber : public RemoteSubscriber {
  friend class DebugTransport;

private:
  TimestampedMsgPtrQueue tmsgp_queue;

private:
  mutable BaseSubscriberQueue::Link by_transport_notify;

public:
  size_t get_queue_length() const;
  size_t get_queue_count() const;

  bool fetch_unsafe(Message *&msgp, Time &timestamp);
  bool notify_unsafe(Message &msg, const Time &timestamp);

  bool fetch(Message *&msgp, Time &timestamp);
  bool notify(Message &msg, const Time &timestamp);

public:
  DebugSubscriber(DebugTransport &transport,
                  TimestampedMsgPtrQueue::Entry queue_buf[],
                  size_t queue_length);
  virtual ~DebugSubscriber();
};


inline
bool DebugSubscriber::notify(Message &msg, const Time &timestamp) {

  return safeguard(notify_unsafe(msg, timestamp));
}


inline
bool DebugSubscriber::fetch(Message *&msgp, Time &timestamp) {

  return safeguard(fetch_unsafe(msgp, timestamp));
}


} // namespace r2p
