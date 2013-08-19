
#include <r2p/BasePublisher.hpp>
#include <r2p/Topic.hpp>
#include <r2p/Message.hpp>

namespace r2p {


bool BasePublisher::alloc_unsafe(Message *&msgp) {

  msgp = topicp->alloc_unsafe();
  if (msgp != NULL) {
    msgp->reset_unsafe();
    return true;
  }
  return false;
}


bool BasePublisher::publish_unsafe(Message &msg) {

  msg.acquire_unsafe();

  Time now = Time::now();
  bool success = topicp->notify_locals_unsafe(msg, now) &&
                 topicp->notify_remotes_unsafe(msg, now);

  if (!msg.release_unsafe()) {
    topicp->free_unsafe(msg);
  }

  return success;
}


bool BasePublisher::publish(Message &msg) {

  msg.acquire();

  Time now = Time::now();
  bool success = topicp->notify_locals(msg, now) &&
                 topicp->notify_remotes(msg, now);

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
