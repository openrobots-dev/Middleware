#pragma once

#include <r2p/common.hpp>
#include <r2p/StaticQueue.hpp>

namespace r2p {

class BaseSubscriber;


class BaseSubscriberQueue : public StaticQueue<BaseSubscriber> {
public:
  BaseSubscriberQueue();
};


} // namespace r2p
