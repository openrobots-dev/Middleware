
#include <r2p/Middleware.hpp>
#include <r2p/NamingTraits.hpp>
#include <r2p/Thread.hpp>
#include <r2p/MgmtMsg.hpp>
#include <r2p/BootloaderMsg.hpp>
#include <r2p/Topic.hpp>
#include <r2p/Node.hpp>
#include <r2p/Transport.hpp>
#include <r2p/Publisher.hpp>
#include <r2p/Subscriber.hpp>

namespace r2p {


void Middleware::initialize(void *info_stackp, size_t info_stacklen,
                            Thread::Priority info_priority) {

  mgmt_threadp = Thread::create_static(info_stackp, info_stacklen,
                                       info_priority, mgmt_threadf, NULL,
                                       "R2P_INFO");
  R2P_ASSERT(mgmt_threadp != NULL);

  // Wait until the info topic is fully initialized
  Thread::Priority oldprio = Thread::get_priority();
  Thread::set_priority(Thread::IDLE);
  SysLock::acquire();
  while (!mgmt_topic.has_local_publishers() ||
         !mgmt_topic.has_local_subscribers()) {
    SysLock::release();
    Thread::yield();
    SysLock::acquire();
  }
  SysLock::release();
  Thread::set_priority(oldprio);
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
    for (StaticList<Node>::Iterator i = nodes.begin(); i != nodes.end(); ++i) {
      i->notify_stop();
    }

    // Stop all remote middlewares
    for (StaticList<Transport>::Iterator i = transports.begin();
         i != transports.end(); ++i) {
      i->notify_stop();
    }

    Thread::Priority oldprio = Thread::get_priority();
    Thread::set_priority(Thread::IDLE);
    Thread::yield();
    Thread::set_priority(oldprio);
  } while (nodes.count() > 0);
}


void Middleware::add(Node &node) {

  nodes.link(node.by_middleware);
}


void Middleware::add(Transport &transport) {

  transports.link(transport.by_middleware);
}


void Middleware::add(Topic &topic) {

  R2P_ASSERT(find_topic(topic.get_name()) == NULL);

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
    i->notify_advertisement(*topicp);
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
    i->notify_advertisement(*topicp);
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
    i->notify_subscription(*topicp);
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

  return topics.find_first(Topic::has_name, namep);
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


Thread::Return Middleware::mgmt_threadf(Thread::Argument) {

  instance.do_mgmt_thread();
  return static_cast<Thread::Return>(0);
}


void Middleware::do_mgmt_thread() {

  MgmtMsg msgbuf[MGMT_BUFFER_LENGTH];
  MgmtMsg *msgqueue_buf[MGMT_BUFFER_LENGTH];
  Subscriber<MgmtMsg> sub(msgqueue_buf, MGMT_BUFFER_LENGTH);
  Publisher<MgmtMsg> pub;
  Node node("R2P_MGMT");

  node.advertise(pub, mgmt_topic.get_name(), Time::INFINITE);
  node.subscribe(sub, mgmt_topic.get_name(), msgbuf);

  do {
    if (node.spin(Time::ms(MGMT_TIMEOUT_MS))) {
      if (is_stopped()) break;

      MgmtMsg *msgp;
      Time deadline;
      while (sub.fetch(msgp, deadline)) {
        switch (msgp->type) {
        case MgmtMsg::CMD_ADVERTISE: {
          Transport *transportp = msgp->pubsub.transportp;
          Topic *topicp = find_topic(msgp->pubsub.topic);
          sub.release(*msgp);
          if (topicp != NULL) {
            transportp->advertise(*topicp);
          }
          break;
        }
        case MgmtMsg::CMD_SUBSCRIBE: {
          Transport *transportp = msgp->pubsub.transportp;
          size_t queue_length = msgp->pubsub.queue_length;
          Topic *topicp = find_topic(msgp->pubsub.topic);
          sub.release(*msgp);
          if (topicp != NULL) {
            transportp->subscribe(*topicp, queue_length);
            transportp->notify_advertisement(*topicp);
          }
          break;
        }
        case MgmtMsg::CMD_GET_NETWORK_STATE: {
          sub.release(*msgp);

          // Send the module info
          while (!pub.alloc(msgp)) {
            Thread::yield();
          }
          ::strncpy(msgp->module.name, get_module_name(),
                    NamingTraits<Middleware>::MAX_LENGTH);
          msgp->module.flags.stopped = safeguard(is_stopped()) ? 1 : 0;
          while (!pub.publish(*msgp)) {
            Thread::yield();
          }

          // Send the (lazy) list of publishers and subscribers
          for (StaticList<Node>::Iterator i = nodes.begin();
               i != nodes.end(); ++i) {
            i->publish_publishers(pub);
            i->publish_subscribers(pub);
          }

          break;
        }
        default: {
          sub.release(*msgp);
          break;
        }
        }
      }
    }

    // TODO: Fine grained iterations by saving iterators; maybe in another function
    // Check for the next unadvertised topic
    if (topic_iter == topics.end()) {
      topics.restart(topic_iter);
    }
    if (topic_iter != topics.end()) {
      Time now = Time::now();
      if (now >= topic_lastiter_time + TOPIC_CHECK_TIMEOUT_MS) {
        topic_lastiter_time = now; // TODO: Add random time
        SysLock::acquire();
        if (topic_iter->is_awaiting_advertisements()) {
          SysLock::release();
          for (StaticList<Transport>::Iterator i = transports.begin();
               i != transports.end(); ++i) {
            i->notify_subscription(*topic_iter);
          }
        } else {
          SysLock::release();
        }
        ++topic_iter;
      }
    }
    Thread::yield();
  } while (!instance.is_stopped());
}


Thread::Return Middleware::boot_threadf(Thread::Argument) {

  instance.do_boot_thread();
  return static_cast<Thread::Return>(0);
}


void Middleware::do_boot_thread() {

  BootloaderMsg msgbuf[BOOT_BUFFER_LENGTH];
  BootloaderMsg *msgqueue_buf[BOOT_BUFFER_LENGTH];
  Subscriber<BootloaderMsg> sub(msgqueue_buf, BOOT_BUFFER_LENGTH);
  Publisher<BootloaderMsg> pub;
  Node node("R2P_BOOT");

  Bootloader::instance.set_page_buffer(
    new Flasher::Data[BOOT_PAGE_LENGTH]
  );

  bool success; (void)success;
  success = node.advertise(pub, boot_topic.get_name(), Time::INFINITE);
  R2P_ASSERT(success);
  success = node.subscribe(sub, boot_topic.get_name(), msgbuf);
  R2P_ASSERT(success);

  for (;;) {
    if (node.spin(Time::ms(1000))) {
      BootloaderMsg *requestp = NULL, *responsep = NULL;
      Time deadline;
      while (sub.fetch(requestp, deadline)) {
        if (pub.alloc(responsep)) {
          if (Bootloader::instance.process(*requestp, *responsep)) {
            sub.release(*requestp);
            pub.publish_remotely(*responsep);
          } else {
            sub.release(*requestp);
            sub.release(*responsep);
          }
        } else {
          requestp->type = BootloaderMsg::NACK;
          pub.publish_remotely(*requestp);
        }
        Thread::yield();
      }
    }
  }
}


Middleware::Middleware(const char *module_namep, const char *bootloader_namep)
:
  module_namep(module_namep),
  mgmt_topic("R2P", sizeof(MgmtMsg)),
  mgmt_threadp(NULL),
  boot_topic(bootloader_namep, sizeof(BootloaderMsg)),
  topic_iter(topics.end()),
  stopped(false)
{
  R2P_ASSERT(is_identifier(module_namep));

  add(mgmt_topic);
}


} // namespace r2p
