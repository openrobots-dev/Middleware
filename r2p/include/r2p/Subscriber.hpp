#pragma once

#include <r2p/common.hpp>
#include <r2p/LocalSubscriber.hpp>

namespace r2p {

class Time;
class Message;


template<typename MessageType>
class Subscriber : public LocalSubscriber {
public:
  typedef bool (*Callback)(const MessageType &msg);

public:
  Callback get_callback() const;
  bool fetch(MessageType *&msgp, Time &timestamp);
  bool release(MessageType &msg);

public:
  Subscriber(MessageType *queue_buf[], size_t queue_length,
             Callback callback = NULL);
  ~Subscriber();
};


template<typename MessageType> inline
bool Subscriber<MessageType>::fetch(MessageType *&msgp, Time &timestamp) {

  static_cast_check<MessageType, Message>();
  return LocalSubscriber::fetch(
    reinterpret_cast<Message *&>(msgp), timestamp
  );
}


template<typename MessageType> inline
typename Subscriber<MessageType>::Callback
Subscriber<MessageType>::get_callback() const {

  static_cast_check<MessageType, Message>();
  return reinterpret_cast<const Callback>(LocalSubscriber::get_callback());
}


template<typename MessageType> inline
bool Subscriber<MessageType>::release(MessageType &msg) {

  static_cast_check<MessageType, Message>();
  return LocalSubscriber::release(static_cast<Message &>(msg));
}


template<typename MessageType> inline
Subscriber<MessageType>::Subscriber(MessageType *queue_buf[],
                                    size_t queue_length, Callback callback)
:
  LocalSubscriber(reinterpret_cast<Message **>(queue_buf), queue_length,
                  reinterpret_cast<LocalSubscriber::Callback>(callback))
{
  static_cast_check<MessageType, Message>();
}


template<typename MessageType> inline
Subscriber<MessageType>::~Subscriber() {

  static_cast_check<MessageType, Message>();
}


} // namespace r2p
