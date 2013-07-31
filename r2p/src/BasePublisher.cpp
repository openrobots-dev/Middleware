
#include <r2p/BasePublisher.hpp>
#include <r2p/Topic.hpp>
#include <r2p/Message.hpp>

namespace r2p {


bool BasePublisher::alloc(Message *&msgp) {

  msgp = topicp->alloc();
  if (msgp != NULL) {
    msgp->reset();
    return true;
  }
  return false;
}


BasePublisher::BasePublisher()
:
  topicp(NULL)
{}


BasePublisher::~BasePublisher() {}


}; // namespace r2p
