
#include "r2p/transport/RTCANPublisher.hpp"

namespace r2p {


RTCANPublisher::RTCANPublisher(Transport &transport)
:
  RemotePublisher(transport)
  {}


RTCANPublisher::~RTCANPublisher() {}


} // namespace r2p
