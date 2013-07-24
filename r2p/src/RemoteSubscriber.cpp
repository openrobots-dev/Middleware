
#include <r2p/RemoteSubscriber.hpp>

namespace r2p {


RemoteSubscriber::RemoteSubscriber(BaseTransport &transport)
:
  BaseSubscriber(),
  transportp(&transport),
  by_transport(*this),
  by_topic(*this)
{}


RemoteSubscriber::~RemoteSubscriber() {}


} // namespace r2p

