
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


bool BasePublisher::alloc(Message *&msgp) {

  SysLock::acquire();
  bool success = alloc_unsafe(msgp);
  SysLock::release();
  return success;
}


BasePublisher::BasePublisher()
:
  topicp(NULL)
{}


BasePublisher::~BasePublisher() {}


}; // namespace r2p
