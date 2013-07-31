
#include <r2p/BasePublisher.hpp>
#include <r2p/Topic.hpp>
#include <r2p/BaseMessage.hpp>

namespace r2p {


Topic *BasePublisher::get_topic() const {

  return topicp;
}


void BasePublisher::notify_advertised(Topic &topic) {

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

  return topicp->notify_local(msg, Time::now());
  return topicp->notify_remote(msg, Time::now());
}


bool BasePublisher::publish_locally(BaseMessage &msg) {

  return topicp->notify_local(msg, Time::now());
}


bool BasePublisher::publish_remotely(BaseMessage &msg) {

  return topicp->notify_remote(msg, Time::now());
}


BasePublisher::BasePublisher()
:
  topicp(NULL)
{}


BasePublisher::~BasePublisher() {}


bool BasePublisher::has_topic(const BasePublisher &pub, const char *namep) {

	return Topic::has_name(*pub.get_topic(), namep);
}


}; // namespace r2p
