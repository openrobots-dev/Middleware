
#include <r2p/transport/DebugPublisher.hpp>

namespace r2p {


DebugPublisher::DebugPublisher(Transport &transport)
:
  RemotePublisher(transport)
  {}


DebugPublisher::~DebugPublisher() {}


} // namespace r2p
