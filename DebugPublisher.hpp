
#ifndef __R2P__DEBUGPUBLISHER_HPP__
#define __R2P__DEBUGPUBLISHER_HPP__

#include <r2p/common.hpp>
#include <r2p/BasePublisher.hpp>
#include <r2p/StaticList.hpp>

namespace r2p {


class DebugPublisher : public BasePublisher {
public:
  StaticList<DebugPublisher>::Link by_transport;

public:
  DebugPublisher();
  ~DebugPublisher();

public:
  static bool has_topic(const DebugPublisher &pub, const char *namep);
};


inline
bool DebugPublisher::has_topic(const DebugPublisher &pub, const char *namep) {

  return Named::has_name(*pub.get_topic(), namep);
}


} // namespace r2p
#endif // __R2P__DEBUGPUBLISHER_HPP__
