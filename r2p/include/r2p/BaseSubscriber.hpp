
#ifndef __R2P__BASESUBSCRIBER_HPP__
#define __R2P__BASESUBSCRIBER_HPP__

#include <r2p/common.hpp>
#include <r2p/MessagePtrQueue.hpp>
#include <r2p/StaticList.hpp>
#include <r2p/MemoryPool.hpp>

namespace r2p {

class Topic;
class BaseMessage;
class Time;


class BaseSubscriber : private Uncopyable {
private:
  Topic *topicp;

public:
  void notify_subscribed(Topic &topic);

  Topic *get_topic() const;

public:
  virtual size_t get_queue_length() const = 0;
  virtual bool notify(BaseMessage &msg, const Time &timestamp) = 0;
  virtual bool fetch(BaseMessage *&msgp, Time &timestamp) = 0;
  bool release(BaseMessage &msg);

protected:
  BaseSubscriber();
  virtual ~BaseSubscriber();

public:
  static bool has_topic(const BaseSubscriber &sub, const char *namep);
};


} // namespace r2p
#endif // __R2P__BASESUBSCRIBER_HPP__
