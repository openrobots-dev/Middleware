
#ifndef __R2P__DEBUGPUBLISHER_HPP__
#define __R2P__DEBUGPUBLISHER_HPP__

#include <r2p/common.hpp>
#include <r2p/RemotePublisher.hpp>
#include <r2p/StaticList.hpp>

namespace r2p {


class DebugPublisher : public RemotePublisher {
public:
  DebugPublisher();
  virtual ~DebugPublisher();
};


} // namespace r2p
#endif // __R2P__DEBUGPUBLISHER_HPP__
