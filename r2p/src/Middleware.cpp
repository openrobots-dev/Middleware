
#include <r2p/Middleware.hpp>
#include <r2p/NamingTraits.hpp>
#include <r2p/Thread.hpp>
#include <r2p/InfoMsg.hpp>
#include <r2p/BootloaderMsg.hpp>
#include <r2p/Topic.hpp>
#include <r2p/Node.hpp>
#include <r2p/Transport.hpp>
#include <r2p/Publisher.hpp>
#include <r2p/Subscriber.hpp>

namespace r2p {


void Middleware::initialize(void *info_stackp, size_t info_stacklen,
                            Thread::Priority info_priority) {

  info_threadp = Thread::create_static(info_stackp, info_stacklen,
                                       info_priority, info_threadf, NULL,
                                       "R2P_INFO");
  R2P_ASSERT(info_threadp != NULL);
}


void Middleware::spin() {

  // TODO process bootloader messages
}


void Middleware::stop() {

  SysLock::acquire();
  stopped = true;
  SysLock::release();

  do {
    // Stop all nodes
    for (StaticList<Node>::Iterator i = nodes.begin();
         i != nodes.end(); ++i) {
      i->itemp->notify_stop();
    }

    // Stop all remote middlewares
    for (StaticList<Transport>::Iterator i = transports.begin();
         i != transports.end(); ++i) {
      i->itemp->notify_stop();
    }

    Thread::Priority oldprio = Thread::get_priority();
    Thread::set_priority(Thread::IDLE);
    Thread::yield();
    Thread::set_priority(oldprio);
  } while (nodes.get_count() > 0);
}


void Middleware::reboot() {

  // TODO: call impl
}


void Middleware::add(Node &node) {

  nodes.link(node.by_middleware);
}


void Middleware::add(Transport &transport) {

  transports.link(transport.by_middleware);
}


void Middleware::add(Topic &topic) {

  topics.link(topic.by_middleware);
}


bool Middleware::advertise(LocalPublisher &pub, const char *namep,
                           const Time &publish_timeout, size_t type_size) {

  Topic *topicp = touch_topic(namep, type_size);
  if (topicp == NULL) return false;
  pub.notify_advertised(*topicp);
  topicp->advertise(pub, publish_timeout);

  for (StaticList<Transport>::Iterator i = transports.begin();
       i != transports.end(); ++i) {
    i->itemp->notify_advertisement(*topicp);
  }

  return true;
}


bool Middleware::advertise(RemotePublisher &pub, const char *namep,
                           const Time &publish_timeout, size_t type_size) {

  Topic *topicp = touch_topic(namep, type_size);
  if (topicp == NULL) return false;
  pub.notify_advertised(*topicp);
  topicp->advertise(pub, publish_timeout);

  for (StaticList<Transport>::Iterator i = transports.begin();
       i != transports.end(); ++i) {
    i->itemp->notify_advertisement(*topicp);
  }

  return true;
}


bool Middleware::subscribe(LocalSubscriber &sub, const char *namep,
                           Message msgpool_buf[], size_t msgpool_buflen,
                           size_t type_size) {

  Topic *topicp = touch_topic(namep, type_size);
  if (topicp == NULL) return false;
  topicp->extend_pool(msgpool_buf, msgpool_buflen);
  sub.notify_subscribed(*topicp);
  topicp->subscribe(sub, msgpool_buflen);

  for (StaticList<Transport>::Iterator i = transports.begin();
       i != transports.end(); ++i) {
    i->itemp->notify_subscription(*topicp);
  }

  return true;
}


bool Middleware::subscribe(RemoteSubscriber &sub, const char *namep,
                           Message msgpool_buf[], size_t msgpool_buflen,
                           size_t type_size) {

  Topic *topicp = touch_topic(namep, type_size);// ce l'ho già
  if (topicp == NULL) return false;
  topicp->extend_pool(msgpool_buf, msgpool_buflen);
  sub.notify_subscribed(*topicp);
  topicp->subscribe(sub, msgpool_buflen);

  return true;
}


Topic *Middleware::find_topic(const char *namep) {

  const StaticList<Topic>::Link *linkp =
    topics.find_first(Topic::has_name, namep);
  return (linkp != NULL) ? linkp->itemp : NULL;
}


Topic *Middleware::touch_topic(const char *namep, size_t type_size) {

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


void Middleware::initialize_info(Node & info_node) {

  add(info_node);
  info_node.advertise(info_pub, "R2P_INFO", Time::INFINITE);
  info_node.subscribe(info_sub, "R2P_INFO", info_msgbuf);
}


void Middleware::spin_info(Node & info_node) {

  if (info_node.spin(Time::ms(INFO_TIMEOUT_MS))) {
    if (is_stopped()) return;

    InfoMsg *infomsgp;
    Time time;
    while (info_sub.fetch(infomsgp, time)) {
      switch (infomsgp->type) {
        case InfoMsg::GET_NETWORK_STATE: {
          info_sub.release(*infomsgp);

          for (StaticList<Node>::Iterator i = nodes.begin();
               i != nodes.end(); ++i) {
            i->itemp->publish_publishers(info_pub);
            i->itemp->publish_subscribers(info_pub);
          }

          break;
        }
        default: {
          info_sub.release(*infomsgp);
          break;
        }
      }
    }
  }
#if 0
  // Check for the next unadvertised topic
  Time now = Time::now();
  if (now >= topic_lastiter_time + TOPIC_CHECK_TIMEOUT_MS) {
    topic_lastiter_time = now; // TODO: Add random time
    if (topic_iter->itemp->is_awaiting_advertisements()) {
      for (StaticList<Transport>::Iterator i = transports.begin();
           i != transports.end(); ++i) {
        i->itemp->notify_subscription(*topic_iter->itemp);
      }
    }
    if (++topic_iter == topics.end()) {
      topics.restart(topic_iter);
    }
  }
#endif
}


Thread::Return Middleware::info_threadf(Thread::Argument) {
  Node info_node("R2P_INFO");

  instance.initialize_info(info_node);
  do {
    instance.spin_info(info_node);
    Thread::yield();
  } while (!instance.is_stopped());

  return static_cast<Thread::Return>(0);
}


Middleware::Middleware(const char *module_namep, const char *bootloader_namep)
:
  module_namep(module_namep),
  info_topic("R2P_INFO", sizeof(InfoMsg)),
  info_threadp(NULL),
  info_sub(info_msgqueue_buf, INFO_BUFFER_LENGTH),
  boot_topic(bootloader_namep, sizeof(BootloaderMsg)),
  topic_iter(topics.end()),
  stopped(false)
{
  R2P_ASSERT(is_identifier(module_namep));

  add(info_topic);
}


} // namespace r2p
