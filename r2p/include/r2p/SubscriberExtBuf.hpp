#pragma once

#include <r2p/common.hpp>
#include <r2p/LocalSubscriber.hpp>

namespace r2p {

class Time;
class Message;


template<typename MessageType>
class SubscriberExtBuf : public LocalSubscriber {
public:
  typedef bool (*Callback)(const MessageType &msg);

public:
  Callback get_callback() const;
  bool fetch(MessageType *&msgp);
  bool fetch(MessageType *&msgp, Time &timestamp);
  bool release(MessageType &msg);

public:
  SubscriberExtBuf(MessageType *queue_buf[], size_t queue_length,
                   Callback callback = NULL);
  ~SubscriberExtBuf();
};


template<typename MT> inline
bool SubscriberExtBuf<MT>::fetch(MT *&msgp) {

  return LocalSubscriber::fetch(
    reinterpret_cast<Message *&>(msgp));
}


template<typename MT> inline
bool SubscriberExtBuf<MT>::fetch(MT *&msgp, Time &timestamp) {

  static_cast_check<MT, Message>();
  return LocalSubscriber::fetch(
    reinterpret_cast<Message *&>(msgp), timestamp
  );
}


template<typename MT> inline
typename SubscriberExtBuf<MT>::Callback
SubscriberExtBuf<MT>::get_callback() const {

  return reinterpret_cast<Callback>(LocalSubscriber::get_callback());
}


template<typename MT> inline
bool SubscriberExtBuf<MT>::release(MT &msg) {

  static_cast_check<MT, Message>();
  return LocalSubscriber::release(static_cast<Message &>(msg));
}


template<typename MT> inline
SubscriberExtBuf<MT>::SubscriberExtBuf(MT *queue_buf[], size_t queue_length,
                                       Callback callback)
:
  LocalSubscriber(reinterpret_cast<Message **>(queue_buf), queue_length,
                  reinterpret_cast<LocalSubscriber::Callback>(callback))
{
  static_cast_check<MT, Message>();
}


template<typename MT> inline
SubscriberExtBuf<MT>::~SubscriberExtBuf() {

  static_cast_check<MT, Message>();
}


} // namespace r2p
