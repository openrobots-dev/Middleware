
#ifndef __R2P__PUBLISHER__HPP__
#define __R2P__PUBLISHER__HPP__

#include <r2p/common.hpp>
#include <r2p/BasePublisher.hpp>
#include <r2p/StaticList.hpp>

namespace r2p {


class Publisher_ : public BasePublisher {
public:
  mutable class ListEntryByNode : private r2p::Uncopyable {
    friend class Publisher_; friend class Node;
    StaticList<Publisher_>::Link entry;
    ListEntryByNode(Publisher_ &pub) : entry(pub) {}
  } by_node;

protected:
  Publisher_();
  ~Publisher_();
};


} // namespace r2p
#endif // __R2P__PUBLISHER__HPP__
