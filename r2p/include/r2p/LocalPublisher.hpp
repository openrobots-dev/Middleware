
#ifndef __R2P__PUBLISHER__HPP__
#define __R2P__PUBLISHER__HPP__

#include <r2p/common.hpp>
#include <r2p/BasePublisher.hpp>
#include <r2p/StaticList.hpp>

namespace r2p {


class LocalPublisher : public BasePublisher {
public:
  mutable class ListEntryByNode : private r2p::Uncopyable {
    friend class LocalPublisher; friend class Node;
    StaticList<LocalPublisher>::Link entry;
    ListEntryByNode(LocalPublisher &pub) : entry(pub) {}
  } by_node;

protected:
  LocalPublisher();
  virtual ~LocalPublisher();
};


} // namespace r2p
#endif // __R2P__PUBLISHER__HPP__
