#pragma once

#include <r2p/common.hpp>
#include <r2p/NamingTraits.hpp>
#include <r2p/impl/MemoryPool_.hpp>
#include <r2p/StaticList.hpp>
#include <r2p/Time.hpp>

namespace r2p {

class Message;
class LocalPublisher;
class LocalSubscriber;
class RemotePublisher;
class RemoteSubscriber;


class Topic : private Uncopyable {
  friend class Middleware;

private:
  const char *const namep;
  Time publish_timeout;
  MemoryPool_ msg_pool;
  size_t num_local_publishers;
  size_t num_remote_publishers;
  StaticList<LocalSubscriber> local_subscribers;
  StaticList<RemoteSubscriber> remote_subscribers;
  size_t max_queue_length;

  StaticList<Topic>::Link by_middleware;

public:
  const char *get_name() const;
  const Time &get_publish_timeout() const;
  size_t get_size() const;
  size_t get_max_queue_length() const;

  bool has_local_publishers() const;
  bool has_remote_publishers() const;
  bool has_local_subscribers() const;
  bool has_remote_subscribers() const;
  bool is_awaiting_advertisements() const;
  bool is_awaiting_subscriptions() const;

  Message *alloc_unsafe();
  template<typename MessageType> bool alloc_unsafe(MessageType *&msgp);
  bool release_unsafe(Message &msg);
  void free_unsafe(Message &msg);
  bool notify_locals_unsafe(Message &msg, const Time &timestamp);
  bool notify_remotes_unsafe(Message &msg, const Time &timestamp);

  Message *alloc();
  template<typename MessageType> bool alloc(MessageType *&msgp);
  bool release(Message &msg);
  void free(Message &msg);
  bool notify_locals(Message &msg, const Time &timestamp);
  bool notify_remotes(Message &msg, const Time &timestamp);
  void extend_pool(Message array[], size_t arraylen);

  void advertise(LocalPublisher &pub, const Time &publish_timeout);
  void advertise(RemotePublisher &pub, const Time &publish_timeout);
  void subscribe(LocalSubscriber &sub, size_t queue_length);
  void subscribe(RemoteSubscriber &sub, size_t queue_length);

public:
  Topic(const char *namep, size_t type_size);

public:
  static bool has_name(const Topic &topic, const char *namep);
};


} // namespace r2p

#include <r2p/LocalPublisher.hpp>
#include <r2p/LocalSubscriber.hpp>
#include <r2p/RemotePublisher.hpp>
#include <r2p/RemoteSubscriber.hpp>

namespace r2p {


inline
const char *Topic::get_name() const {

  return namep;
}


inline
const Time &Topic::get_publish_timeout() const {

  return publish_timeout;
}


inline
size_t Topic::get_size() const {

  return msg_pool.get_item_size() - sizeof(Message);
}


inline
size_t Topic::get_max_queue_length() const {

  return max_queue_length;
}


inline
bool Topic::has_local_subscribers() const {

  return !local_subscribers.is_empty_unsafe();
}


inline
bool Topic::has_remote_subscribers() const {

  return !remote_subscribers.is_empty_unsafe();
}


inline
bool Topic::has_local_publishers() const {

  return num_local_publishers > 0;
}


inline
bool Topic::has_remote_publishers() const {

  return num_remote_publishers > 0;
}


inline
bool Topic::is_awaiting_advertisements() const {

  return !has_local_publishers() && !has_remote_publishers() &&
         has_local_subscribers();
}


inline
bool Topic::is_awaiting_subscriptions() const {

  return !has_local_subscribers() && !has_remote_subscribers() &&
         has_local_publishers();
}


inline
Message *Topic::alloc_unsafe() {

  return reinterpret_cast<Message *>(msg_pool.alloc_unsafe());
}


template<typename MessageType> inline
bool Topic::alloc_unsafe(MessageType *&msgp) {

  static_cast_check<MessageType, Message>();
  return (msgp = reinterpret_cast<MessageType *>(alloc_unsafe())) != NULL;
}


inline
bool Topic::release_unsafe(Message &msg) {

  if (!msg.release_unsafe()) {
    free_unsafe(msg);
    return true;
  }
  return false;
}


inline
void Topic::free_unsafe(Message &msg) {

  msg_pool.free_unsafe(reinterpret_cast<void *>(&msg));
}


inline
Message *Topic::alloc() {

  return reinterpret_cast<Message *>(msg_pool.alloc());
}


template<typename MessageType> inline
bool Topic::alloc(MessageType *&msgp) {

  static_cast_check<MessageType, Message>();
  return (msgp = reinterpret_cast<MessageType *>(alloc())) != NULL;
}


inline
bool Topic::release(Message &msg) {

  SysLock::acquire();
  bool freed = release_unsafe(msg);
  SysLock::release();
  return freed;
}


inline
void Topic::free(Message &msg) {

  msg_pool.free(reinterpret_cast<void *>(&msg));
}


inline
bool Topic::notify_locals(Message &msg, const Time &timestamp) {

  SysLock::acquire();
  bool success = notify_locals_unsafe(msg, timestamp);
  SysLock::release();
  return success;
}


inline
bool Topic::notify_remotes(Message &msg, const Time &timestamp) {

  SysLock::acquire();
  bool success = notify_remotes_unsafe(msg, timestamp);
  SysLock::release();
  return success;
}


inline
void Topic::extend_pool(Message array[], size_t arraylen) {

  msg_pool.extend(array, arraylen);
}


inline
bool Topic::has_name(const Topic &topic, const char *namep) {

  return namep != NULL && 0 == strncmp(topic.get_name(), namep,
                                       NamingTraits<Topic>::MAX_LENGTH);
}


} // namespace r2p
