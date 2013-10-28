
#include <r2p/BasePublisher.hpp>
#include <r2p/Topic.hpp>
#include <r2p/Message.hpp>

namespace r2p {


bool BasePublisher::alloc_unsafe(Message *&msgp) {

  R2P_ASSERT(topicp != NULL);

  msgp = topicp->alloc_unsafe();
  return msgp != NULL;
}


bool BasePublisher::publish_unsafe(Message &msg) {

  R2P_ASSERT(topicp != NULL);

  msg.acquire_unsafe();

  Time now = Time::now();
  bool success;
  success = topicp->notify_locals_unsafe(msg, now);
  success = topicp->notify_remotes_unsafe(msg, now) && success;

  if (!msg.release_unsafe()) {
    topicp->free_unsafe(msg);
  }

  return success;
}


bool BasePublisher::publish(Message &msg) {

  R2P_ASSERT(topicp != NULL);

  msg.acquire();

  Time now = Time::now();
  bool success;
  success = topicp->notify_locals(msg, now);
  success = topicp->notify_remotes(msg, now) && success;

  if (!msg.release()) {
    topicp->free(msg);
  }

  return success;
}


BasePublisher::BasePublisher()
:
  topicp(NULL)
{}


BasePublisher::~BasePublisher() {}


}; // namespace r2p
