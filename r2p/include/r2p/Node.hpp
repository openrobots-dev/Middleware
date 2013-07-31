#pragma once

#include <r2p/common.hpp>
#include <r2p/NamingTraits.hpp>
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
class Message;


class Node : private Uncopyable {
  friend class Middleware;

private:
  const char *const namep;
  StaticList<LocalPublisher> publishers;
  StaticList<LocalSubscriber> subscribers;
  SpinEvent event;
  Time timeout;

  mutable StaticList<Node>::Link by_middleware;

public:
  const char *get_name() const;

  template<typename Message>
  bool advertise(Publisher<Message> &pub, const char *namep,
                 const Time &publish_timeout = Time::INFINITE);

  template<typename Message>
  bool subscribe(Subscriber<Message> &sub, const char *namep,
                 Message msgpool_buf[]);

  void publish_publishers(Publisher<InfoMsg> &info_pub);
  void publish_subscribers(Publisher<InfoMsg> &info_pub);

  bool notify_unsafe(unsigned event_index);
  bool notify_stop_unsafe();

  bool notify(unsigned event_index);
  bool notify_stop();

  bool spin(const Time &timeout = Time::INFINITE);

private:
  bool advertise(LocalPublisher &pub, const char *namep,
                 const Time &publish_timeout, size_t msg_size);
  bool subscribe(LocalSubscriber &sub, const char *namep,
                 Message msgpool_buf[], size_t msg_size);

public:
  Node(const char *namep);

public:
  static bool has_name(const Node &node, const char *namep);
};


inline
const char *Node::get_name() const {

  return namep;
}


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
                   reinterpret_cast<Message *>(msgpool_buf),
                   sizeof(Message));
}


inline
bool Node::notify_unsafe(unsigned event_index) {

  return event.signal_unsafe(event_index);
}


inline
bool Node::notify_stop_unsafe() {

  // Just signal a dummy unlikely event
  return event.signal_unsafe(SpinEvent::MAX_INDEX);
}


inline
bool Node::notify(unsigned event_index) {

  return event.signal(event_index);
}


inline
bool Node::notify_stop() {

  // Just signal a dummy unlikely event
  return event.signal(SpinEvent::MAX_INDEX);
}


inline
bool Node::has_name(const Node &node, const char *namep) {

  return namep != NULL && ::strncmp(node.get_name(), namep,
                                    NamingTraits<Node>::MAX_LENGTH);
}


} // namespace r2p
