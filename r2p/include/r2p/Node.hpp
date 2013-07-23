
#ifndef __R2P__NODE_HPP__
#define __R2P__NODE_HPP__

#include <r2p/common.hpp>
#include <r2p/StaticList.hpp>
#include <r2p/SpinEvent.hpp>
#include <r2p/Time.hpp>
#include <r2p/InfoMsg.hpp>

namespace r2p {

class Topic;
class Publisher_;
class Subscriber_;
template<typename Message> class Publisher;
template<typename Message> class Subscriber;
class BaseMessage;


class Node : public Named, private Uncopyable {
private:
  StaticList<Publisher_> publishers;
  StaticList<Subscriber_> subscribers;
  SpinEvent event;
  Time timeout;

public:
  mutable class ListEntryByMiddleware : private r2p::Uncopyable {
    friend class Node; friend class Middleware;
    StaticList<Node>::Link entry;
    ListEntryByMiddleware(Node &node) : entry(node) {}
  } by_middleware;

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
  bool advertise(Publisher_ &pub, const char *namep,
                 const Time &publish_timeout, size_t msg_size);
  bool subscribe(Subscriber_ &sub, const char *namep,
                 BaseMessage msgpool_buf[], size_t msg_size);

public:
  Node(const char *namep);
};


template<typename Message> inline
bool Node::advertise(Publisher<Message> &pub, const char *namep,
                     const Time &publish_timeout) {

  return advertise(reinterpret_cast<Publisher_ &>(pub), namep,
                   publish_timeout, sizeof(Message));
}


template<typename Message> inline
bool Node::subscribe(Subscriber<Message> &sub, const char *namep,
                     Message msgpool_buf[]) {

  return subscribe(reinterpret_cast<Subscriber_ &>(sub), namep,
                   reinterpret_cast<BaseMessage *>(msgpool_buf),
                   sizeof(Message));
}


inline
bool Node::notify(unsigned event_index) {

  return event.signal(event_index);
}


} // namespace r2p
#endif // __R2P__NODE_HPP__
