
#ifndef __R2P__BASEPUBLISHER_HPP__
#define __R2P__BASEPUBLISHER_HPP__

#include <r2p/common.hpp>

namespace r2p {

class BaseMessage;
class Topic;


class BasePublisher : private Uncopyable {
private:
  Topic *topicp;

public:
  Topic *get_topic() const;

  void notify_advertised(Topic &topic);

  bool alloc(BaseMessage *&msgp);
  bool publish(BaseMessage &msg);
  bool publish_locally(BaseMessage &msg);
  bool publish_remotely(BaseMessage &msg);

protected:
  BasePublisher();
  virtual ~BasePublisher();

public:
  static bool has_topic(const BasePublisher &pub, const char *namep);
};


} // namespace r2p
#endif // __R2P__BASEPUBLISHER_HPP__
