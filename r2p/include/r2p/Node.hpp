
#ifndef __R2P__NODE_HPP__
#define __R2P__NODE_HPP__

#include <r2p/common.hpp>
#include <r2p/StaticList.hpp>
#include <r2p/SpinEvent.hpp>
#include <r2p/Time.hpp>
#include <r2p/InfoMsg.hpp>

namespace r2p {

class Topic;
class LocalPublisher;
class LocalSubscriber;
template<typename Message> class Publisher;
template<typename Message> class Subscriber;
class BaseMessage;


class Node : public Named, private Uncopyable {
  friend class Middleware;

private:
  StaticList<LocalPublisher> publishers;
  StaticList<LocalSubscriber> subscribers;
  SpinEvent event;
  Time timeout;

  StaticList<Node>::Link by_middleware;

public:
  template<typename Message>
  bool advertise(Publisher<Message> &pub, const char *namep,
                 const Time &publish_timeout = Time::INFINITE);

  template<typename Message>
  bool subscribe(Subscriber<Message> &sub, const char *namep,
                 Message msgpool_buf[]);

  void publish_publishers(Publisher<InfoMsg> &info_pub);
  void publish_subscribers(Publisher<InfoMsg> &info_pub);

  bool notify(unsigned event_index);
  bool spin(const Time &timeout = Time::INFINITE);

private:
  bool advertise(LocalPublisher &pub, const char *namep,
                 const Time &publish_timeout, size_t msg_size);
  bool subscribe(LocalSubscriber &sub, const char *namep,
                 BaseMessage msgpool_buf[], size_t msg_size);

public:
  Node(const char *namep);
};


template<typename Message> inline
bool Node::advertise(Publisher<Message> &pub, const char *namep,
                     const Time &publish_timeout) {

  return advertise(reinterpret_cast<LocalPublisher &>(pub), namep,
                   publish_timeout, sizeof(Message));
}


template<typename Message> inline
bool Node::subscribe(Subscriber<Message> &sub, const char *namep,
                     Message msgpool_buf[]) {

  return subscribe(reinterpret_cast<LocalSubscriber &>(sub), namep,
                   reinterpret_cast<BaseMessage *>(msgpool_buf),
                   sizeof(Message));
}


inline
bool Node::notify(unsigned event_index) {

  return event.signal(event_index);
}


} // namespace r2p
#endif // __R2P__NODE_HPP__
