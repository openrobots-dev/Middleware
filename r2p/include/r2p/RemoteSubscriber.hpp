#pragma once

#include <r2p/common.hpp>
#include <r2p/BaseSubscriber.hpp>

namespace r2p {

class Transport;


class RemoteSubscriber : public BaseSubscriber {
  friend class Transport;
  friend class Topic;

private:
  Transport *transportp;

  mutable StaticList<RemoteSubscriber>::Link by_transport;
  mutable StaticList<RemoteSubscriber>::Link by_topic;

protected:
  Transport *get_transport() const;

protected:
  RemoteSubscriber(Transport &transport);
  virtual ~RemoteSubscriber() = 0;
};


inline
Transport *RemoteSubscriber::get_transport() const {

  return transportp;
}


} // namespace r2p
