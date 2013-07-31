#pragma once

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
