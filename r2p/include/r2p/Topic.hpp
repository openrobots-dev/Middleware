
#ifndef __R2P__TOPIC_HPP__
#define __R2P__TOPIC_HPP__

#include <r2p/common.hpp>
#include <r2p/impl/MemoryPool_.hpp>
#include <r2p/StaticList.hpp>
#include <r2p/Time.hpp>

namespace r2p {


#if !defined(R2P_TOPIC_MAXNAMELEN) || defined(__DOXYGEN__)
#define R2P_TOPIC_MAXNAMELEN 63
#endif

#if R2P_TOPIC_MAXNAMELEN < 1 || R2P_TOPIC_MAXNAMELEN > 254
#error "R2P_TOPIC_MAXNAMELEN should be in range 1..254"
#endif


class BaseMessage;
class BaseSubscriber;
class BasePublisher;


class Topic : public Named, private Uncopyable {
private:
  Time publish_timeout;
  MemoryPool_ msg_pool;
  StaticList<BaseSubscriber> local_subscribers;
  StaticList<BaseSubscriber> remote_subscribers;
  size_t num_publishers;

public:
  mutable class ListEntryByMiddleware : private r2p::Uncopyable {
    friend class Topic; friend class Middleware;
    StaticList<Topic>::Link entry;
    ListEntryByMiddleware(Topic &topic) : entry(topic) {}
  } by_middleware;

public:
  size_t get_num_publishers_unsafe() const;
  const Time &get_publish_timeout() const;
  size_t get_msg_size() const;
  size_t get_num_publishers() const;
  void extend_pool(BaseMessage array[], size_t arraylen);

  BaseMessage *alloc();
  void free(BaseMessage &msg);
  bool notify_locally(BaseMessage &msg, const Time &timestamp);
  bool notify_remotely(BaseMessage &msg, const Time &timestamp);

  void advertise_cb(BasePublisher &pub, const Time &publish_timeout);
  void subscribe_local_cb(BaseSubscriber &sub);
  void subscribe_remote_cb(BaseSubscriber &sub);

public:
  Topic(const char *namep, size_t type_size);

public:
  static bool has_name(const Topic &topic, const char *namep);
};


} // namespace r2p

#include <r2p/BaseSubscriber.hpp>
#include <r2p/BasePublisher.hpp>

namespace r2p {


inline
size_t Topic::get_num_publishers_unsafe() const {

  return num_publishers;
}


inline
const Time &Topic::get_publish_timeout() const {

  return publish_timeout;
}


inline
size_t Topic::get_msg_size() const {

  return msg_pool.get_block_length();
}


inline
void Topic::extend_pool(BaseMessage array[], size_t arraylen) {

  msg_pool.grow(array, arraylen);
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
void Topic::subscribe_local_cb(BaseSubscriber &sub) {

  local_subscribers.link(sub.by_topic);
}


inline
void Topic::subscribe_remote_cb(BaseSubscriber &sub) {

  remote_subscribers.link(sub.by_topic);
}


inline
bool Topic::has_name(const Topic &topic, const char *namep) {

  return Named::has_name(topic, namep);
}


} // namespace r2p
#endif // __R2P__TOPIC_HPP__
