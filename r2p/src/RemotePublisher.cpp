
#include <r2p/RemotePublisher.hpp>

namespace r2p {

RemotePublisher::RemotePublisher(Transport &transport)
:
  BasePublisher(),
  transportp(&transport),
  by_transport(*this)
{}


RemotePublisher::~RemotePublisher() {}


} // namespace r2p

