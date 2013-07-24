
#ifndef __R2P__SUBSCRIBER__HPP__
#define __R2P__SUBSCRIBER__HPP__

#include <r2p/common.hpp>
#include <r2p/BaseSubscriber.hpp>
#include <r2p/StaticList.hpp>
#include <r2p/MessagePtrQueue.hpp>


namespace r2p {

class Node;
class BaseMessage;


class LocalSubscriber : public BaseSubscriber {
  friend class Node;
  friend class Topic;

public:
  class Callback {
  private:
    virtual bool action(const BaseMessage &msg) const = 0;

  public:
    bool operator () (const BaseMessage &msg) const {
      return action(msg);
    }

    bool operator () (const BaseMessage &msg) {
      return operator () (msg);
    }

  protected:
    Callback() {}

  public:
    virtual ~Callback() {}
  };

private:
  Node *nodep;
  const Callback *callbackp;
  MessagePtrQueue msgp_queue;
  uint_least8_t event_index;

  StaticList<LocalSubscriber>::Link by_node;
  StaticList<LocalSubscriber>::Link by_topic;

public:
  const Callback *get_callback() const;
  size_t get_queue_length() const;

public:
  bool fetch(BaseMessage *&msgp, Time &timestamp);
  bool notify(BaseMessage &msg, const Time &timestamp);

protected:
  LocalSubscriber(BaseMessage *queue_buf[], size_t queue_length,
                  const Callback *callbackp = NULL);
  virtual ~LocalSubscriber();
};


} // namespace r2p
#endif // __R2P__SUBSCRIBER__HPP__
