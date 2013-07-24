
#ifndef __R2P__SUBSCRIBER_HPP__
#define __R2P__SUBSCRIBER_HPP__

#include <r2p/common.hpp>
#include <r2p/LocalSubscriber.hpp>

namespace r2p {

class Time;
class BaseMessage;


template<typename Message>
class Subscriber : public LocalSubscriber {
public:
  class Callback : public LocalSubscriber::Callback {
  public:
    virtual bool action (const Message &msg) const = 0;

  private:
    bool action(const BaseMessage &msg) const {
      return action(static_cast<const Message &>(msg));
    }

  protected:
    Callback() : LocalSubscriber::Callback() {}
    virtual ~Callback() {}
  };

public:
  const Callback &get_callback() const;
  bool fetch(Message *&msgp, Time &timestamp);
  bool release(Message &msg);

public:
  Subscriber(Message *queue_buf[], size_t queue_length,
             const Callback *callbackp = NULL);
};


template<typename Message> inline
bool Subscriber<Message>::fetch(Message *&msgp, Time &timestamp) {

  return LocalSubscriber::fetch(
    reinterpret_cast<BaseMessage *&>(msgp), timestamp
  );
}


template<typename Message> inline
const typename Subscriber<Message>::Callback &
Subscriber<Message>::get_callback() const {

  return reinterpret_cast<const Callback &>(LocalSubscriber::get_callback());
}


template<typename Message> inline
bool Subscriber<Message>::release(Message &msg) {

  return LocalSubscriber::release(reinterpret_cast<BaseMessage &>(msg));
}


template<typename Message> inline
Subscriber<Message>::Subscriber(Message *queue_buf[], size_t queue_length,
                                const Callback *callbackp) :

  LocalSubscriber(reinterpret_cast<BaseMessage **>(queue_buf), queue_length,
                  callbackp)
{}


} // namespace r2p
#endif // __R2P__SUBSCRIBER_HPP__
