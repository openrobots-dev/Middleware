
#ifndef __R2P__BASESUBSCRIBERQUEUE_HPP__
#define __R2P__BASESUBSCRIBERQUEUE_HPP__

#include <r2p/common.hpp>
#include <r2p/StaticQueue.hpp>

namespace r2p {

class BaseSubscriber;


class BaseSubscriberQueue : public StaticQueue<BaseSubscriber> {
public:
  BaseSubscriberQueue();
};


} // namespace r2p
#endif // __R2P__BASESUBSCRIBERQUEUE_HPP__
