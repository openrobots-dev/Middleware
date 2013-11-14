#pragma once

#include <r2p/common.hpp>
#include <r2p/SubscriberExtBuf.hpp>

namespace r2p {

class Time;
class Message;
class Node;


template<typename MessageType, unsigned QUEUE_LENGTH>
class Subscriber : public SubscriberExtBuf<MessageType> {
  friend class Node;

public:
  typedef typename SubscriberExtBuf<MessageType>::Callback Callback;

private:
  MessageType msgpool_buf[QUEUE_LENGTH];
  MessageType *queue_buf[QUEUE_LENGTH];

public:
  Subscriber(Callback callback = NULL);
  ~Subscriber();
};


template<typename MT, unsigned QL> inline
Subscriber<MT, QL>::Subscriber(Callback callback)
:
  SubscriberExtBuf<MT>(queue_buf, QL, callback)
{}


template<typename MT, unsigned QL> inline
Subscriber<MT, QL>::~Subscriber() {}


} // namespace r2p
