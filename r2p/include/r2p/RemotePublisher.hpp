
#ifndef __R2P__REMOTEPUBLISHER_HPP__
#define __R2P__REMOTEPUBLISHER_HPP__

#include <r2p/common.hpp>
#include <r2p/BasePublisher.hpp>
#include <r2p/StaticList.hpp>

namespace r2p {


class RemotePublisher : public BasePublisher {
  friend class BaseTransport;

private:
  StaticList<RemotePublisher>::Link by_transport;

protected:
  RemotePublisher();
  virtual ~RemotePublisher();

};


} // namespace r2p
#endif // __R2P__REMOTEPUBLISHER_HPP__
