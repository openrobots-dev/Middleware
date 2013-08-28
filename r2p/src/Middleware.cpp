
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


void Middleware::initialize(void *mgmt_boot_stackp, size_t mgmt_boot_stacklen,
                            Thread::Priority mgmt_boot_priority) {

  R2P_ASSERT(mgmt_boot_stackp != NULL);
  R2P_ASSERT(mgmt_boot_stacklen > 0);

  this->mgmt_boot_stackp = mgmt_boot_stackp;
  this->mgmt_boot_stacklen = mgmt_boot_stacklen;
  this->mgmt_boot_priority = mgmt_boot_priority;

  mgmt_boot_threadp = Thread::create_static(
    mgmt_boot_stackp, mgmt_boot_stacklen, mgmt_boot_priority,
    mgmt_threadf, NULL, "R2P_MGMT"
  );
  R2P_ASSERT(mgmt_boot_threadp != NULL);

  // Wait until the info topic is fully initialized
  Thread::Priority oldprio = Thread::get_priority();
  Thread::set_priority(Thread::IDLE);
  SysLock::acquire();
  while (!mgmt_topic.has_local_publishers() ||
         !mgmt_topic.has_local_subscribers()) {
    SysLock::release();
    Thread::sleep(Time::ms(500));
    SysLock::acquire();
  }
  SysLock::release();
  Thread::set_priority(oldprio);
}


void Middleware::stop() {

  bool trigger = false;
  SysLock::acquire();
  if (!stopped) {
    trigger = stopped = true;
  }
  SysLock::release();

  // Stop all remote middlewares, and force remote bootloader pubs/subs
  for (StaticList<Transport>::Iterator i = transports.begin();
       i != transports.end(); ++i) {
    bool success; (void)success;
    success = i->notify_stop();
    R2P_ASSERT(success);
    success = i->touch_publisher(boot_topic);
    R2P_ASSERT(success);
    success = i->touch_subscriber(boot_topic, BOOT_BUFFER_LENGTH);
    R2P_ASSERT(success);
  }

  SysLock::acquire();
  while (num_running_nodes > 0) {
    SysLock::release();

    // Stop all nodes
    for (StaticList<Node>::Iterator i = nodes.begin(); i != nodes.end(); ++i) {
      i->notify_stop();
    }

    Thread::sleep(Time::ms(500)); // TODO: Configure delay
    SysLock::acquire();
  }
  SysLock::release();

  // Enter bootloader mode
  if (trigger) {
    trigger = Thread::join(*mgmt_boot_threadp);
    R2P_ASSERT(trigger);
    mgmt_boot_threadp = Thread::create_static(
      mgmt_boot_stackp,mgmt_boot_stacklen, mgmt_boot_priority,
      boot_threadf, NULL, "R2P_BOOT"
    );
    R2P_ASSERT(mgmt_boot_threadp != NULL);
  }
}


void Middleware::add(Node &node) {

  SysLock::acquire();
  nodes.link_unsafe(node.by_middleware);
  ++num_running_nodes;
  SysLock::release();
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


void Middleware::confirm_stop(Node &node) {

  SysLock::acquire();
  R2P_ASSERT(num_running_nodes > 0);
  R2P_ASSERT(nodes.contains_unsafe(node));
  --num_running_nodes;
  nodes.unlink_unsafe(node.by_middleware);
  SysLock::release();
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
  return Thread::OK;
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
          R2P_ASSERT(transportp != NULL);
          Topic *topicp = find_topic(msgp->pubsub.topic);
          sub.release(*msgp);
          if (topicp != NULL) {
            transportp->advertise(*topicp);
          }
          break;
        }
        case MgmtMsg::CMD_SUBSCRIBE: {
          Transport *transportp = msgp->pubsub.transportp;
          R2P_ASSERT(transportp != NULL);
          size_t queue_length = msgp->pubsub.queue_length;
          Topic *topicp = find_topic(msgp->pubsub.topic);
          sub.release(*msgp);
          if (topicp != NULL) {
            transportp->subscribe(*topicp, queue_length);
          }
          break;
        }
        case MgmtMsg::CMD_GET_NETWORK_STATE: {
          sub.release(*msgp);

          // Send the module info
          while (!pub.alloc(msgp)) {
            Thread::yield();
          }
          strncpy(msgp->module.name, get_module_name(),
                  NamingTraits<Middleware>::MAX_LENGTH);
          SysLock::acquire();
          msgp->module.flags.stopped = (is_stopped() != false);
          SysLock::release();
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
  return Thread::OK;
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
    if (node.spin(Time::ms(1000))) { // TODO: Configuration
      BootloaderMsg *requestp = NULL, *responsep = NULL;
      Time deadline;
      while (sub.fetch(requestp, deadline)) {
        if (pub.alloc(responsep)) {
          responsep->cleanup();
          if (Bootloader::instance.process(*requestp, *responsep)) {
            sub.release(*requestp);
            while (!pub.publish_remotely(*responsep)) {
              Thread::sleep(Time::ms(100)); // TODO: Configuration
            }
          } else {
            sub.release(*requestp);
            sub.release(*responsep);
          }
        } else {
          requestp->type = BootloaderMsg::NACK;
          while (!pub.publish_remotely(*requestp)) {
            Thread::sleep(Time::ms(100)); // TODO: Configuration
          }
        }
        Thread::yield();
      }
    } else {
      BootloaderMsg *heartbeatp = NULL;
      if (pub.alloc(heartbeatp)) {
        heartbeatp->cleanup();
        heartbeatp->type = BootloaderMsg::HEARTBEAT;
        if (!pub.publish_remotely(*heartbeatp)) {
          sub.release(*heartbeatp);
        }
      }
    }
  }
}


Middleware::Middleware(const char *module_namep, const char *bootloader_namep)
:
  module_namep(module_namep),
  mgmt_boot_stackp(NULL),
  mgmt_boot_stacklen(0),
  mgmt_boot_priority(Thread::LOWEST),
  mgmt_topic("R2P", sizeof(MgmtMsg)),
  mgmt_boot_threadp(NULL),
  boot_topic(bootloader_namep, sizeof(BootloaderMsg)),
  topic_iter(topics.end()),
  stopped(false),
  num_running_nodes(0)
{
  R2P_ASSERT(is_identifier(module_namep));

  add(mgmt_topic);
  add(boot_topic);
}


} // namespace r2p
