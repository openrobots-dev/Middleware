#pragma once

#include <r2p/common.hpp>
#include <r2p/LocalPublisher.hpp>

namespace r2p {

class Message;


template<typename MessageType>
class Publisher : public LocalPublisher {
public:
  bool alloc(MessageType *&msgp);
  bool publish(MessageType &msg);

public:
  Publisher();
  ~Publisher();
};


template<typename MessageType> inline
bool Publisher<MessageType>::alloc(MessageType *&msgp) {

  static_cast_check<MessageType, Message>();
  return BasePublisher::alloc(reinterpret_cast<Message *&>(msgp));
}


template<typename MessageType> inline
bool Publisher<MessageType>::publish(MessageType &msg) {

  static_cast_check<MessageType, Message>();
  return BasePublisher::publish(static_cast<Message &>(msg));
}


template<typename MessageType> inline
Publisher<MessageType>::Publisher() {

  static_cast_check<MessageType, Message>();
}


template<typename MessageType> inline
Publisher<MessageType>::~Publisher() {

  static_cast_check<MessageType, Message>();
}


} // namespace r2p
