
#ifndef __R2P__REMOTESUBSCRIBER_HPP__
#define __R2P__REMOTESUBSCRIBER_HPP__

#include <r2p/common.hpp>
#include <r2p/BaseSubscriber.hpp>

namespace r2p {

class BaseTransport;


class RemoteSubscriber : public BaseSubscriber {
  friend class BaseTransport;
  friend class Topic;

private:
  BaseTransport *transportp;

  StaticList<RemoteSubscriber>::Link by_transport;
  StaticList<RemoteSubscriber>::Link by_topic;

protected:
  BaseTransport *get_transport() const;

protected:
  RemoteSubscriber(BaseTransport &transport);
  virtual ~RemoteSubscriber();
};


inline
BaseTransport *RemoteSubscriber::get_transport() const {

  return transportp;
}


} // namespace r2p
#endif // __R2P__REMOTESUBSCRIBER_HPP__
