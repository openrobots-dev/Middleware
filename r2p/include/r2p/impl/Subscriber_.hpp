
#ifndef __R2P__SUBSCRIBER__HPP__
#define __R2P__SUBSCRIBER__HPP__

#include <r2p/common.hpp>
#include <r2p/BaseSubscriber.hpp>
#include <r2p/StaticList.hpp>
#include <r2p/MessagePtrQueue.hpp>


namespace r2p {

class Node;
class BaseMessage;


class Subscriber_ : public BaseSubscriber {
  friend class Node;

public:
  typedef bool (*Callback)(const BaseMessage &msg);

private:
  Node *nodep;
  Callback callback;
  MessagePtrQueue msgp_queue;
  uint_least8_t event_index;

public:
  mutable class ListEntryByNode : private r2p::Uncopyable {
    friend class Subscriber_; friend class Node;
    StaticList<Subscriber_>::Link entry;
    ListEntryByNode(Subscriber_ &sub) : entry(sub) {}
  } by_node;

protected:
  Callback get_callback() const;
  size_t get_queue_length() const;

public:
  bool fetch(BaseMessage *&msgp, Time &timestamp);
  bool notify(BaseMessage &msg, const Time &timestamp);

protected:
  Subscriber_(BaseMessage *queue_buf[], size_t queue_length,
              Callback callback = NULL);
  ~Subscriber_();
};


} // namespace r2p
#endif // __R2P__SUBSCRIBER__HPP__
