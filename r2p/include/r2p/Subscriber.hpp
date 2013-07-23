
#ifndef __R2P__SUBSCRIBER_HPP__
#define __R2P__SUBSCRIBER_HPP__

#include <r2p/common.hpp>
#include "impl/Subscriber_.hpp"

namespace r2p {

class Time;
class BaseMessage;


template<typename Message>
class Subscriber : public Subscriber_ {
public:
  typedef bool (*Callback)(const Message &msg);

public:
  Callback get_callback() const;
  bool fetch(Message *&msgp, Time &timestamp);
  bool release(Message &msg);

public:
  Subscriber(Message *queue_buf[], size_t queue_length,
             Callback callback = NULL);
};


template<typename Message> inline
bool Subscriber<Message>::fetch(Message *&msgp, Time &timestamp) {

  return Subscriber_::fetch(
    reinterpret_cast<BaseMessage *&>(msgp), timestamp
  );
}


template<typename Message> inline
typename Subscriber<Message>::Callback Subscriber<Message>::get_callback()
const {

  return reinterpret_cast<Callback &>(Subscriber_::get_callback());
}


template<typename Message> inline
bool Subscriber<Message>::release(Message &msg) {

  return Subscriber_::release(reinterpret_cast<BaseMessage &>(msg));
}


template<typename Message> inline
Subscriber<Message>::Subscriber(Message *queue_buf[], size_t queue_length,
                                Callback callback) :

  Subscriber_(reinterpret_cast<BaseMessage **>(queue_buf), queue_length,
              reinterpret_cast<Subscriber_::Callback>(callback))
{}


} // namespace r2p
#endif // __R2P__SUBSCRIBER_HPP__
