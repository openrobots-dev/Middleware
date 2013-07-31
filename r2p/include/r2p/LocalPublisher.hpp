#pragma once

#include <r2p/common.hpp>
#include <r2p/BasePublisher.hpp>
#include <r2p/StaticList.hpp>

namespace r2p {


class LocalPublisher : public BasePublisher {
  friend class Node;

private:
  mutable StaticList<LocalPublisher>::Link by_node;

protected:
  LocalPublisher();
  virtual ~LocalPublisher();
};


} // namespace r2p
