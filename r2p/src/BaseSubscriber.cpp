
#include <r2p/BaseSubscriber.hpp>
#include <r2p/Message.hpp>
#include <r2p/Topic.hpp>

namespace r2p {


bool BaseSubscriber::release_unsafe(Message &msg) {

  R2P_ASSERT(topicp != NULL);

  if (!msg.release_unsafe()) {
    topicp->free_unsafe(msg);
    return false;
  }
  return true;
}


bool BaseSubscriber::release(Message &msg) {

  R2P_ASSERT(topicp != NULL);

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
