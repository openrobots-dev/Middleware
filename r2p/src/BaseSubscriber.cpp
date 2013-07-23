
#include <r2p/BaseSubscriber.hpp>
#include <r2p/BaseMessage.hpp>
#include <r2p/Topic.hpp>

namespace r2p {


void BaseSubscriber::subscribe_cb(Topic &topic) {

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
  topicp(NULL),
  by_topic(*this)
{}


}; // namespace r2p
