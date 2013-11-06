#pragma once

#include <r2p/common.hpp>
#include <r2p/NamingTraits.hpp>
#include <r2p/impl/MemoryPool_.hpp>
#include <r2p/StaticList.hpp>
#include <r2p/Time.hpp>

namespace r2p {

#if !defined(R2P_DEFAULT_FORWARDING) || defined(__DOXYGEN__)
#define R2P_DEFAULT_FORWARDING_RULE     R2P_USE_BRIDGE_MODE
#endif


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
#if R2P_USE_BRIDGE_MODE
  bool forwarding;
#endif

  StaticList<Topic>::Link by_middleware;

public:
  const char *get_name() const;
  const Time &get_publish_timeout() const;
  size_t get_type_size() const;
  size_t get_payload_size() const;
  size_t get_max_queue_length() const;
  bool is_forwarding() const;

  bool has_local_publishers() const;
  bool has_remote_publishers() const;
  bool has_publishers() const;
  bool has_local_subscribers() const;
  bool has_remote_subscribers() const;
  bool has_subscribers() const;
  bool is_awaiting_advertisements() const;
  bool is_awaiting_subscriptions() const;

  const Time compute_deadline_unsafe(const Time &timestamp) const;
  const Time compute_deadline_unsafe() const;
  Message *alloc_unsafe();
  template<typename MessageType> bool alloc_unsafe(MessageType *&msgp);
  bool release_unsafe(Message &msg);
  void free_unsafe(Message &msg);
  bool notify_locals_unsafe(Message &msg, const Time &timestamp);
  bool notify_remotes_unsafe(Message &msg, const Time &timestamp);
  bool forward_copy_unsafe(const Message &msg, const Time &timestamp);


  const Time compute_deadline(const Time &timestamp) const;
  const Time compute_deadline() const;
  Message *alloc();
  template<typename MessageType> bool alloc(MessageType *&msgp);
  bool release(Message &msg);
  void free(Message &msg);
  bool notify_locals(Message &msg, const Time &timestamp);
  bool notify_remotes(Message &msg, const Time &timestamp);
  bool forward_copy(const Message &msg, const Time &timestamp);
  void extend_pool(Message array[], size_t arraylen);

  void advertise(LocalPublisher &pub, const Time &publish_timeout);
  void advertise(RemotePublisher &pub, const Time &publish_timeout);
  void subscribe(LocalSubscriber &sub, size_t queue_length);
  void subscribe(RemoteSubscriber &sub, size_t queue_length);

private:
  void patch_pubsub_msg(Message &msg, Transport &transport) const;

public:
  Topic(const char *namep, size_t type_size,
        bool forwarding = R2P_DEFAULT_FORWARDING_RULE);

public:
  static bool has_name(const Topic &topic, const char *namep);
};


class MessageGuard : private Uncopyable {
private:
  Message &msg;
  Topic &topic;

public:
  MessageGuard(Message &msg, Topic &topic);
  ~MessageGuard();
};


class MessageGuardUnsafe : private Uncopyable {
private:
  Message &msg;
  Topic &topic;

public:
  MessageGuardUnsafe(Message &msg, Topic &topic);
  ~MessageGuardUnsafe();
};


} // namespace r2p

#include <r2p/Message.hpp>
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
size_t Topic::get_type_size() const {

  return msg_pool.get_item_size();
}


inline
size_t Topic::get_payload_size() const {

  return Message::get_payload_size(msg_pool.get_item_size());
}


inline
size_t Topic::get_max_queue_length() const {

  return max_queue_length;
}


inline
bool Topic::is_forwarding() const {

#if R2P_USE_BRIDGE_MODE
  return forwarding;
#else
  return R2P_DEFAULT_FORWARDING_RULE;
#endif
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
bool Topic::has_publishers() const {

  return num_local_publishers > 0 || num_remote_publishers > 0;
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
bool Topic::has_subscribers() const {

  return !local_subscribers.is_empty_unsafe() ||
         !remote_subscribers.is_empty_unsafe();
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
const Time Topic::compute_deadline_unsafe(const Time &timestamp) const {

  return timestamp + publish_timeout;
}


inline
const Time Topic::compute_deadline_unsafe() const {

  return compute_deadline_unsafe(Time::now());
}


inline
Message *Topic::alloc_unsafe() {

  register Message *msgp = reinterpret_cast<Message *>(msg_pool.alloc_unsafe());
  if (msgp != NULL) {
    msgp->reset_unsafe();
    return msgp;
  }
  return NULL;
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
const Time Topic::compute_deadline(const Time &timestamp) const {

  SysLock::acquire();
  const Time &deadline = compute_deadline_unsafe(timestamp);
  SysLock::release();
  return deadline;
}


inline
const Time Topic::compute_deadline() const {

  return compute_deadline(Time::now());
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
void Topic::extend_pool(Message array[], size_t arraylen) {

  msg_pool.extend(array, arraylen);
}


inline
bool Topic::has_name(const Topic &topic, const char *namep) {

  return namep != NULL && 0 == strncmp(topic.get_name(), namep,
                                       NamingTraits<Topic>::MAX_LENGTH);
}


inline
MessageGuard::MessageGuard(Message &msg, Topic &topic)
:
  msg(msg),
  topic(topic)
{
  msg.acquire();
}


inline
MessageGuard::~MessageGuard() {

  topic.release(msg);
}


inline
MessageGuardUnsafe::MessageGuardUnsafe(Message &msg, Topic &topic)
:
  msg(msg),
  topic(topic)
{
  msg.acquire_unsafe();
}


inline
MessageGuardUnsafe::~MessageGuardUnsafe() {

  topic.release_unsafe(msg);
}


} // namespace r2p
