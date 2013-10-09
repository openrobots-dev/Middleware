#pragma once

#include <r2p/common.hpp>
#include <r2p/MessagePtrQueue.hpp>
#include <r2p/StaticList.hpp>
#include <r2p/MemoryPool.hpp>

namespace r2p {

class Topic;
class Message;
class Time;


class BaseSubscriber : private Uncopyable {
private:
  Topic *topicp;

public:
  Topic *get_topic() const;
  virtual size_t get_queue_length() const = 0;
  void notify_subscribed(Topic &topic);

  virtual bool notify_unsafe(Message &msg, const Time &timestamp) = 0;
  virtual bool fetch_unsafe(Message *&msgp, Time &timestamp) = 0;
  bool release_unsafe(Message &msg);

  virtual bool notify(Message &msg, const Time &timestamp) = 0;
  virtual bool fetch(Message *&msgp, Time &timestamp) = 0;
  bool release(Message &msg);

protected:
  BaseSubscriber();
  virtual ~BaseSubscriber() = 0;

public:
  static bool has_topic(const BaseSubscriber &sub, const char *namep);
};


inline
Topic *BaseSubscriber::get_topic() const {

  return topicp;
}


inline
void BaseSubscriber::notify_subscribed(Topic &topic) {

  R2P_ASSERT(topicp == NULL);

  topicp = &topic;
}


} // namespace r2p
