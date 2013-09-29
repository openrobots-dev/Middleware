#pragma once

#include <r2p/common.hpp>
#include <r2p/Message.hpp>

namespace r2p {

class Message;
class Topic;


class BasePublisher : private Uncopyable {
private:
  Topic *topicp;

public:
  const Topic *get_topic() const;

  void notify_advertised(Topic &topic);

  bool alloc_unsafe(Message *&msgp);
  bool publish_unsafe(Message &msg);
  bool publish_locally_unsafe(Message &msg);
  bool publish_remotely_unsafe(Message &msg);

  bool alloc(Message *&msgp);
  bool publish(Message &msg);
  bool publish_locally(Message &msg);
  bool publish_remotely(Message &msg);

protected:
  BasePublisher();
  virtual ~BasePublisher() = 0;

public:
  static bool has_topic(const BasePublisher &pub, const char *namep);
};


} // namespace r2p

#include <r2p/Topic.hpp>

namespace r2p {


inline
const Topic *BasePublisher::get_topic() const {

  return topicp;
}


inline
void BasePublisher::notify_advertised(Topic &topic) {

  R2P_ASSERT(topicp == NULL);

  topicp = &topic;
}


inline
bool BasePublisher::publish_locally_unsafe(Message &msg) {

  R2P_ASSERT(topicp != NULL);

  return topicp->notify_locals_unsafe(msg, Time::now());
}


inline
bool BasePublisher::publish_remotely_unsafe(Message &msg) {

  R2P_ASSERT(topicp != NULL);

  return topicp->notify_remotes_unsafe(msg, Time::now());
}


inline
bool BasePublisher::alloc(Message *&msgp) {

  SysLock::acquire();
  bool success = alloc_unsafe(msgp);
  SysLock::release();
  return success;
}


inline
bool BasePublisher::publish_locally(Message &msg) {

  R2P_ASSERT(topicp != NULL);

  return topicp->notify_locals_unsafe(msg, Time::now());
}


inline
bool BasePublisher::publish_remotely(Message &msg) {

  R2P_ASSERT(topicp != NULL);

  return topicp->notify_remotes(msg, Time::now());
}


inline
bool BasePublisher::has_topic(const BasePublisher &pub, const char *namep) {

  return pub.topicp != NULL && Topic::has_name(*pub.topicp, namep);
}


} // namespace r2p
