#pragma once

#include <r2p/common.hpp>
#include <r2p/LocalSubscriber.hpp>

namespace r2p {

class Time;
class Message;


template<typename MessageType>
class Subscriber : public LocalSubscriber {
public:
  class Callback : public LocalSubscriber::Callback {
  public:
    virtual bool action (const MessageType &msg) const = 0;

  private:
    bool action(const Message &msg) const {
      return action(static_cast<const MessageType &>(msg));
    }

  protected:
    Callback() : LocalSubscriber::Callback() {}
    virtual ~Callback() {}
  };

public:
  const Callback &get_callback() const;
  bool fetch(MessageType *&msgp, Time &timestamp);
  bool release(MessageType &msg);

public:
  Subscriber(MessageType *queue_buf[], size_t queue_length,
             const Callback *callbackp = NULL);
};


template<typename MessageType> inline
bool Subscriber<MessageType>::fetch(MessageType *&msgp, Time &timestamp) {

  return LocalSubscriber::fetch(
    reinterpret_cast<Message *&>(msgp), timestamp
  );
}


template<typename MessageType> inline
const typename Subscriber<MessageType>::Callback &
Subscriber<MessageType>::get_callback() const {

  return reinterpret_cast<const Callback &>(LocalSubscriber::get_callback());
}


template<typename MessageType> inline
bool Subscriber<MessageType>::release(MessageType &msg) {

  return LocalSubscriber::release(reinterpret_cast<Message &>(msg));
}


template<typename MessageType> inline
Subscriber<MessageType>::Subscriber(MessageType *queue_buf[],
                                    size_t queue_length,
                                    const Callback *callbackp) :

  LocalSubscriber(reinterpret_cast<Message **>(queue_buf), queue_length,
                  callbackp)
{}


} // namespace r2p
