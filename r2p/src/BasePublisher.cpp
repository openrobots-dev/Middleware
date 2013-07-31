
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


<<<<<<< HEAD
bool BasePublisher::has_topic(const BasePublisher &pub, const char *namep) {

	return Topic::has_name(*pub.get_topic(), namep);
}


=======
>>>>>>> origin/v2
}; // namespace r2p
