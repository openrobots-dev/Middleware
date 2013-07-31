
#ifndef __R2P__RTCANSUBSCRIBER_HPP__
#define __R2P__RTCANSUBSCRIBER_HPP__

#include <r2p/common.hpp>
#include <r2p/RemoteSubscriber.hpp>
#include <r2p/TimestampedMsgPtrQueue.hpp>
#include <r2p/StaticList.hpp>
#include <r2p/BaseSubscriberQueue.hpp>
#include <ch.h>

namespace r2p {

class BaseMessage;
class RTCANTransport;


class RTCANSubscriber : public RemoteSubscriber {
  friend class RTCANTransport;

private:
  size_t queue_free;

public:
  bool fetch(BaseMessage *&msgp, Time &timestamp);
  bool notify(BaseMessage &msg, const Time &timestamp);

public:
  RTCANSubscriber(RTCANTransport &transport, size_t queue_length);
  virtual ~RTCANSubscriber();
};


} // namespace r2p

#endif // __R2P__RTCANSUBSCRIBER_HPP__
