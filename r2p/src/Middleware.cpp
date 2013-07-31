
#include <r2p/Middleware.hpp>
#include <r2p/NamingTraits.hpp>
#include <r2p/Thread.hpp>
#include <r2p/InfoMsg.hpp>
#include <r2p/BootloaderMsg.hpp>
#include <r2p/Topic.hpp>
#include <r2p/Node.hpp>
#include <r2p/BaseTransport.hpp>
#include <r2p/Publisher.hpp>
#include <r2p/Subscriber.hpp>

namespace r2p {


void Middleware::initialize(void *info_stackp, size_t info_stacklen,
                            Thread::Priority info_priority) {

  add(info_topic);

  info_threadp = Thread::create_static(info_stackp, info_stacklen,
                                       info_priority, info_threadf, NULL,
                                       "R2P_INFO");
  R2P_ASSERT(info_threadp != NULL);

  Thread::Priority oldprio = Thread::get_priority();
  Thread::set_priority(Thread::IDLE);
  do {
    Thread::yield();
  } while (nodes.get_count() < 1);
  Thread::set_priority(oldprio);
}


void Middleware::spin() {

  // TODO process bootloader messages
}


void Middleware::stop() {

  SysLock::acquire();
  stopped = true;
  SysLock::release();

  for (StaticList<BaseTransport>::Iterator i = transports.begin();
       i != transports.end(); ++i) {
    i->datap->notify_stop();
  }

  Thread::Priority oldprio = Thread::get_priority();
  Thread::set_priority(Thread::IDLE);
  do {
    Thread::yield();
  } while (nodes.get_count() > 1);
  Thread::set_priority(oldprio);
}


void Middleware::reboot() {

  // TODO: call impl
}


void Middleware::add(Node &node) {

  R2P_ASSERT(NULL == nodes.find_first(Named::has_name, node.get_name()));

  nodes.link(node.by_middleware);
}


void Middleware::add(BaseTransport &transport) {

  R2P_ASSERT(NULL == transports.find_first(Named::has_name,
                                           transport.get_name()));

  transports.link(transport.by_middleware);
}


void Middleware::add(Topic &topic) {

  R2P_ASSERT(NULL == topics.find_first(Named::has_name, topic.get_name()));

  topics.link(topic.by_middleware);
}


bool Middleware::advertise(LocalPublisher &pub, const char *namep,
                           const Time &publish_timeout, size_t type_size) {

  R2P_ASSERT(namep != NULL);
  R2P_ASSERT(namep[0] != 0);

  Topic *topicp = touch_topic(namep, type_size);
  if (topicp == NULL) return false;
  pub.notify_advertised(*topicp);
  topicp->advertise(pub, publish_timeout);

  for (StaticList<BaseTransport>::Iterator i = transports.begin();
       i != transports.end(); ++i) {
    i->datap->notify_advertisement(*topicp);
  }

  return true;
}


bool Middleware::advertise(RemotePublisher &pub, const char *namep,
                           const Time &publish_timeout, size_t type_size) {

  R2P_ASSERT(namep != NULL);
  R2P_ASSERT(namep[0] != 0);

  Topic *topicp = touch_topic(namep, type_size);
  if (topicp == NULL) return false;
  pub.notify_advertised(*topicp);
  topicp->advertise(pub, publish_timeout);

  for (StaticList<BaseTransport>::Iterator i = transports.begin();
       i != transports.end(); ++i) {
    i->datap->notify_advertisement(*topicp);
  }

  return true;
}


bool Middleware::subscribe(LocalSubscriber &sub, const char *namep,
                           BaseMessage msgpool_buf[], size_t msgpool_buflen,
                           size_t type_size) {

  R2P_ASSERT(namep != NULL);
  R2P_ASSERT(namep[0] != 0);

  Topic *topicp = touch_topic(namep, type_size);
  if (topicp == NULL) return false;
  topicp->extend_pool(msgpool_buf, msgpool_buflen);
  sub.notify_subscribed(*topicp);
  topicp->subscribe(sub);

  for (StaticList<BaseTransport>::Iterator i = transports.begin();
       i != transports.end(); ++i) {
    i->datap->notify_subscription(*topicp, sub.get_queue_length());
  }

  return true;
}


bool Middleware::subscribe(RemoteSubscriber &sub, const char *namep,
                           BaseMessage msgpool_buf[], size_t msgpool_buflen,
                           size_t type_size) {

  R2P_ASSERT(namep != NULL);
  R2P_ASSERT(namep[0] != 0);

  Topic *topicp = touch_topic(namep, type_size);
  if (topicp == NULL) return false;
  topicp->extend_pool(msgpool_buf, msgpool_buflen);
  sub.notify_subscribed(*topicp);
  topicp->subscribe(sub);

  return true;
}


Topic *Middleware::find_topic(const char *namep) {

  // TODO topic.has_name()
  const StaticList<Topic>::Link *linkp =
    topics.find_first(Named::has_name, namep);
  return (linkp != NULL) ? linkp->datap : NULL;
}


Topic *Middleware::touch_topic(const char *namep, size_t type_size) {

  R2P_ASSERT(namep != NULL);
  R2P_ASSERT(namep[0] != 0);

  // Check if a topic exists with the given name
  Topic *topicp = find_topic(namep);
  if (topicp != NULL) {
    return topicp;
  }

  // Allocate a new topic
  topicp = new Topic(namep, type_size);
  if (topicp != NULL) {
    topics.link(topicp->by_middleware);
  }
  return topicp;
}


Thread::ReturnType Middleware::info_threadf(Thread::ArgumentType) {

  enum { INFO_BUFFER_LENGTH = 2 };
  InfoMsg info_msgbuf[INFO_BUFFER_LENGTH];
  InfoMsg *info_msgqueue_buf[INFO_BUFFER_LENGTH];
  Subscriber<InfoMsg> info_sub(info_msgqueue_buf, INFO_BUFFER_LENGTH);
  Publisher<InfoMsg> info_pub;
  Node info_node("R2P_INFO");

  info_node.advertise(info_pub, "R2P_INFO", Time::INFINITE);
  info_node.subscribe(info_sub, "R2P_INFO", info_msgbuf);
  instance.add(info_node);

  for (;;) {
    info_node.spin();

    InfoMsg *infomsgp;
    Time time;
    while (info_sub.fetch(infomsgp, time)) {
      switch (infomsgp->type) {
        case InfoMsg::GET_NETWORK_STATE: {
          info_sub.release(*infomsgp);

          for (StaticList<Node>::Iterator i = instance.nodes.begin();
               i != instance.nodes.end(); ++i) {
            i->datap->publish_publishers(info_pub);
            i->datap->publish_subscribers(info_pub);
          }

          break;
        }
        default: {
          info_sub.release(*infomsgp);
          break;
        }
      }
    }
    Thread::yield();
  }
  return static_cast<Thread::ReturnType>(0);
}


Middleware::Middleware(const char *module_namep, const char *bootloader_namep)
:
  Named(module_namep),
  info_topic("R2P_INFO", sizeof(InfoMsg)),
  info_threadp(NULL),
  boot_topic(bootloader_namep, sizeof(BootloaderMsg)),
  stopped(false)
{
  R2P_ASSERT(::strlen(module_namep) <= NamingTraits<Middleware>::MAX_LENGTH);
}


} // namespace r2p
