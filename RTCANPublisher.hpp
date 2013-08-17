
#ifndef __R2P__RTCANPUBLISHER_HPP__
#define __R2P__RTCANPUBLISHER_HPP__

#include <r2p/common.hpp>
#include <r2p/RemotePublisher.hpp>
#include <r2p/StaticList.hpp>

#include "rtcan.h"

namespace r2p {


class RTCANPublisher : public RemotePublisher {

public:
  rtcan_msg_t rtcan_header; // FIXME should be private

  RTCANPublisher();
  virtual ~RTCANPublisher();
};


} // namespace r2p
#endif // __R2P__RTCANPUBLISHER_HPP__
