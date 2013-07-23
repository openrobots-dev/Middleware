
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
protected:
  Topic *topicp;

public:
  StaticList<BaseSubscriber>::Link by_topic;

public:
  void subscribe_cb(Topic &topic);

  Topic *get_topic() const;

public:
  virtual size_t get_queue_length() const = 0;
  virtual bool notify(BaseMessage &msg, const Time &timestamp) = 0;
  virtual bool fetch(BaseMessage *&msgp, Time &timestamp) = 0;
  bool release(BaseMessage &msg);

protected:
  BaseSubscriber();

public:
  virtual ~BaseSubscriber() {};
};


} // namespace r2p
#endif // __R2P__BASESUBSCRIBER_HPP__
