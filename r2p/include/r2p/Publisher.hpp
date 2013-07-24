
#ifndef __R2P__PUBLISHER_HPP__
#define __R2P__PUBLISHER_HPP__

#include <r2p/common.hpp>
#include <r2p/LocalPublisher.hpp>

namespace r2p {

class BaseMessage;


template<typename Message>
class Publisher : public LocalPublisher {
public:
  bool alloc(Message *&msgp);
  bool publish(Message &msg);
};


template<typename Message> inline
bool Publisher<Message>::alloc(Message *&msgp) {

  return BasePublisher::alloc(reinterpret_cast<BaseMessage *&>(msgp));
}


template<typename Message> inline
bool Publisher<Message>::publish(Message &msg) {

  return BasePublisher::publish(reinterpret_cast<BaseMessage &>(msg));
}


} // namespace r2p
#endif // __R2P__PUBLISHER_HPP__
