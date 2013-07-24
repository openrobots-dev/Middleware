
#include <r2p/RemotePublisher.hpp>

namespace r2p {


RemotePublisher::RemotePublisher()
:
  BasePublisher(),
  by_transport(*this)
{}


RemotePublisher::~RemotePublisher() {}


} // namespace r2p

