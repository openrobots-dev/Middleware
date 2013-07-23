
#include <r2p/BasePublisher.hpp>
#include <r2p/BaseMessage.hpp>

namespace r2p {


Topic *BasePublisher::get_topic() const {

  return topicp;
}


void BasePublisher::advertise_cb(Topic &topic) {

  R2P_ASSERT(topicp == NULL);

  topicp = &topic;
}


bool BasePublisher::alloc(BaseMessage *&msgp) {

  msgp = topicp->alloc();
  if (msgp != NULL) {
    msgp->reset();
    return true;
  }
  return false;
}


bool BasePublisher::publish(BaseMessage &msg) {

  return topicp->notify_locally(msg, Time::now());
  return topicp->notify_remotely(msg, Time::now());
}


bool BasePublisher::publish_locally(BaseMessage &msg) {

  return topicp->notify_locally(msg, Time::now());
}


bool BasePublisher::publish_remotely(BaseMessage &msg) {

  return topicp->notify_remotely(msg, Time::now());
}


BasePublisher::BasePublisher()
:
  topicp(NULL)
{}


}; // namespace r2p
