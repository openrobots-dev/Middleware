
#include <r2p/BaseSubscriber.hpp>
#include <r2p/BaseMessage.hpp>
#include <r2p/Topic.hpp>

namespace r2p {


void BaseSubscriber::notify_subscribed(Topic &topic) {

  R2P_ASSERT(topicp == NULL);

  topicp = &topic;
}


Topic *BaseSubscriber::get_topic() const {

  return topicp;
}


bool BaseSubscriber::release(BaseMessage &msg) {

  if (!msg.release()) {
    topicp->free(msg);
    return false;
  }
  return true;
}


BaseSubscriber::BaseSubscriber()
:
  topicp(NULL)
{}


BaseSubscriber::~BaseSubscriber() {}


bool BaseSubscriber::has_topic(const BaseSubscriber &sub, const char *namep) {

  return Topic::has_name(*sub.topicp, namep);
}


}; // namespace r2p
