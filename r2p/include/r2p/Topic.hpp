
#ifndef __R2P__TOPIC_HPP__
#define __R2P__TOPIC_HPP__

#include <r2p/common.hpp>
#include <r2p/impl/MemoryPool_.hpp>
#include <r2p/StaticList.hpp>
#include <r2p/Time.hpp>

namespace r2p {


class BaseMessage;
class LocalPublisher;
class LocalSubscriber;
class RemotePublisher;
class RemoteSubscriber;


class Topic : public Named, private Uncopyable {
  friend class Middleware;

private:
  Time publish_timeout;
  MemoryPool_ msg_pool;
  size_t num_local_publishers;
  size_t num_remote_publishers;
  StaticList<LocalSubscriber> local_subscribers;
  StaticList<RemoteSubscriber> remote_subscribers;

  StaticList<Topic>::Link by_middleware;

public:
  const Time &get_publish_timeout() const;
  size_t get_size() const;
  void extend_pool(BaseMessage array[], size_t arraylen);
  bool has_local_publishers() const;
  bool has_remote_publishers() const;
  bool has_local_subscribers() const;
  bool has_remote_subscribers() const;

  BaseMessage *alloc();
  void free(BaseMessage &msg);
  bool notify_local(BaseMessage &msg, const Time &timestamp);
  bool notify_remote(BaseMessage &msg, const Time &timestamp);

  void advertise(LocalPublisher &pub, const Time &publish_timeout);
  void advertise(RemotePublisher &pub, const Time &publish_timeout);
  void subscribe(LocalSubscriber &sub);
  void subscribe(RemoteSubscriber &sub);

public:
  Topic(const char *namep, size_t type_size);
};


} // namespace r2p

#include <r2p/LocalPublisher.hpp>
#include <r2p/LocalSubscriber.hpp>
#include <r2p/RemotePublisher.hpp>
#include <r2p/RemoteSubscriber.hpp>

namespace r2p {


inline
const Time &Topic::get_publish_timeout() const {

  return publish_timeout;
}


inline
size_t Topic::get_size() const {

  return msg_pool.get_block_length();
}


inline
void Topic::extend_pool(BaseMessage array[], size_t arraylen) {

  msg_pool.grow(array, arraylen);
}


inline
bool Topic::has_local_subscribers() const {

  return !local_subscribers.is_empty();
}


inline
bool Topic::has_remote_subscribers() const {

  return !remote_subscribers.is_empty();
}


inline
BaseMessage *Topic::alloc() {

  return reinterpret_cast<BaseMessage *>(msg_pool.alloc());
}


inline
void Topic::free(BaseMessage &msg) {

  msg_pool.free(reinterpret_cast<void *>(&msg));
}


inline
void Topic::subscribe(LocalSubscriber &sub) {

  local_subscribers.link(sub.by_topic);
}


inline
void Topic::subscribe(RemoteSubscriber &sub) {

  remote_subscribers.link(sub.by_topic);
}


} // namespace r2p
#endif // __R2P__TOPIC_HPP__
