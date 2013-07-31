#pragma once

#include <r2p/common.hpp>
#include <r2p/BasePublisher.hpp>
#include <r2p/StaticList.hpp>

namespace r2p {


class RemotePublisher : public BasePublisher {
  friend class Transport;

private:
  mutable StaticList<RemotePublisher>::Link by_transport;

protected:
  RemotePublisher();
  virtual ~RemotePublisher();

};


} // namespace r2p
