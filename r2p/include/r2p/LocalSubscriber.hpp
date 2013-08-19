#pragma once

#include <r2p/common.hpp>
#include <r2p/BaseSubscriber.hpp>
#include <r2p/StaticList.hpp>
#include <r2p/MessagePtrQueue.hpp>


namespace r2p {

class Node;
class Message;


class LocalSubscriber : public BaseSubscriber {
  friend class Node;
  friend class Topic;

public:
  typedef bool (*Callback)(const Message &msg);

private:
  Node *nodep;
  const Callback callback;
  MessagePtrQueue msgp_queue;
  uint_least8_t event_index;

  mutable StaticList<LocalSubscriber>::Link by_node;
  mutable StaticList<LocalSubscriber>::Link by_topic;

public:
  Callback get_callback() const;
  size_t get_queue_length() const;

public:
  bool fetch_unsafe(Message *&msgp, Time &timestamp);
  bool notify_unsafe(Message &msg, const Time &timestamp);

  bool fetch(Message *&msgp, Time &timestamp);
  bool notify(Message &msg, const Time &timestamp);

protected:
  LocalSubscriber(Message *queue_buf[], size_t queue_length,
                  Callback callback = NULL);
  virtual ~LocalSubscriber();
};


} // namespace r2p

#include <r2p/Node.hpp>

namespace r2p {


inline
LocalSubscriber::Callback LocalSubscriber::get_callback() const {

  return callback;
}


inline
size_t LocalSubscriber::get_queue_length() const {

  return msgp_queue.get_length();
}


inline
bool LocalSubscriber::fetch_unsafe(Message *&msgp, Time &timestamp) {

  if (msgp_queue.fetch_unsafe(msgp)) {
    timestamp = Time::now();
    return true;
  }
  return false;
}


inline
bool LocalSubscriber::notify_unsafe(Message &msg, const Time &timestamp) {

  (void)timestamp;
  if (msgp_queue.post_unsafe(&msg)) {
    nodep->notify_unsafe(event_index);
    return true;
  }
  return false;
}


} // namespace r2p
