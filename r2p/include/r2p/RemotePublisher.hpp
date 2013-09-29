#pragma once

#include <r2p/common.hpp>
#include <r2p/BasePublisher.hpp>
#include <r2p/StaticList.hpp>

namespace r2p {


class RemotePublisher : public BasePublisher {
  friend class Transport;

private:
  Transport *transportp;

  mutable StaticList<RemotePublisher>::Link by_transport;

public:
  Transport *get_transport() const;

protected:
  RemotePublisher(Transport &transport);
  virtual ~RemotePublisher() = 0;

};


inline
Transport *RemotePublisher::get_transport() const {

  return transportp;
}


} // namespace r2p
