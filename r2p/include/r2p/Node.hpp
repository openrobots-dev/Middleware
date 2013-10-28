#pragma once

#include <r2p/common.hpp>
#include <r2p/NamingTraits.hpp>
#include <r2p/StaticList.hpp>
#include <r2p/SpinEvent.hpp>
#include <r2p/Time.hpp>
#include <r2p/MgmtMsg.hpp>

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
  const StaticList<LocalPublisher> &get_publishers() const;
  const StaticList<LocalSubscriber> &get_subscribers() const;
  bool get_enabled() const;
  void set_enabled(bool enabled);

  template<typename MessageType>
  bool advertise(Publisher<MessageType> &pub, const char *namep,
                 const Time &publish_timeout = Time::INFINITE);

  template<typename MessageType>
  bool subscribe(Subscriber<MessageType> &sub, const char *namep,
                 MessageType msgpool_buf[]);

  void notify_unsafe(unsigned event_index);
  void notify_stop_unsafe();

  void notify(unsigned event_index);
  void notify_stop();

  bool spin(const Time &timeout = Time::INFINITE);

private:
  bool advertise(LocalPublisher &pub, const char *namep,
                 const Time &publish_timeout, size_t msg_size);
  bool subscribe(LocalSubscriber &sub, const char *namep,
                 Message msgpool_buf[], size_t msg_size);

public:
  Node(const char *namep, bool enabled = true);
  ~Node();

public:
  static bool has_name(const Node &node, const char *namep);
};


} // namespace r2p

#include <r2p/Thread.hpp>

namespace r2p {


inline
const char *Node::get_name() const {

  return namep;
}


inline
const StaticList<LocalPublisher> &Node::get_publishers() const {

  return publishers;
}


inline
const StaticList<LocalSubscriber> &Node::get_subscribers() const {

  return subscribers;
}


inline
bool Node::get_enabled() const {

  return event.get_thread() != NULL;
}


inline
void Node::set_enabled(bool enabled) {

  event.set_thread(enabled ? &Thread::self() : NULL);
}


template<typename MessageType> inline
bool Node::advertise(Publisher<MessageType> &pub, const char *namep,
                     const Time &publish_timeout) {

  return advertise(pub, namep, publish_timeout, sizeof(MessageType));
}


template<typename MessageType> inline
bool Node::subscribe(Subscriber<MessageType> &sub, const char *namep,
                     MessageType msgpool_buf[]) {

  return subscribe(sub, namep, msgpool_buf, sizeof(MessageType));
}


inline
void Node::notify_unsafe(unsigned event_index) {

  event.signal_unsafe(event_index);
}


inline
void Node::notify_stop_unsafe() {

  // Just signal a dummy unlikely event
  event.signal_unsafe(SpinEvent::MAX_INDEX);
}


inline
void Node::notify(unsigned event_index) {

  event.signal(event_index);
}


inline
void Node::notify_stop() {

  // Just signal a dummy unlikely event
  event.signal(SpinEvent::MAX_INDEX);
}


inline
bool Node::has_name(const Node &node, const char *namep) {

  return namep != NULL &&
         0 == strncmp(node.get_name(), namep, NamingTraits<Node>::MAX_LENGTH);
}


} // namespace r2p
