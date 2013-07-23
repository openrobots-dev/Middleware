
#ifndef __R2P__BASEPUBLISHER_HPP__
#define __R2P__BASEPUBLISHER_HPP__

#include <r2p/common.hpp>
#include <r2p/StaticList.hpp>
#include <r2p/Topic.hpp>

namespace r2p {

class BaseMessage;
class Topic;


class BasePublisher : private Uncopyable {
protected:
  Topic *topicp;

public:
  Topic *get_topic() const;

  void advertise_cb(Topic &topic);

  bool alloc(BaseMessage *&msgp);
  bool publish(BaseMessage &msg);
  bool publish_locally(BaseMessage &msg);
  bool publish_remotely(BaseMessage &msg);

protected:
  BasePublisher();
public:
  virtual ~BasePublisher() {};
};


} // namespace r2p
#endif // __R2P__BASEPUBLISHER_HPP__
