
#include <r2p/Middleware.hpp>
#include <r2p/NamingTraits.hpp>
#include <r2p/Thread.hpp>
#include <r2p/MgmtMsg.hpp>
#include <r2p/BootMsg.hpp>
#include <r2p/Bootloader.hpp>
#include <r2p/Topic.hpp>
#include <r2p/Node.hpp>
#include <r2p/Transport.hpp>
#include <r2p/Publisher.hpp>
#include <r2p/Subscriber.hpp>
#include <r2p/ScopedLock.hpp>

namespace r2p {


uint32_t Middleware::rebooted_magic R2P_NORESET;
uint32_t Middleware::boot_mode_magic R2P_NORESET;


void Middleware::initialize(void *mgmt_stackp, size_t mgmt_stacklen,
                            Thread::Priority mgmt_priority,
                            void *boot_stackp, size_t boot_stacklen,
                            Thread::Priority boot_priority) {

  R2P_ASSERT(mgmt_stackp != NULL);
  R2P_ASSERT(mgmt_stacklen > 0);

  lists_lock.initialize();

  this->mgmt_stackp = mgmt_stackp;
  this->mgmt_stacklen = mgmt_stacklen;
  this->mgmt_priority = mgmt_priority;
  this->boot_stackp = boot_stackp;
  this->boot_stacklen = boot_stacklen;
  this->boot_priority = boot_priority;

#if R2P_USE_BOOTLOADER
  if (is_bootloader_mode()) {
    R2P_ASSERT(boot_stackp != NULL);
    R2P_ASSERT(boot_stacklen > 0);

    topics.link(boot_topic.by_middleware);
  }
#endif
  topics.link(mgmt_topic.by_middleware);
}


void Middleware::start() {

#if R2P_USE_BOOTLOADER
  if (is_bootloader_mode()) {
    R2P_ASSERT(boot_stackp != NULL);

    boot_threadp = Thread::create_static(
      boot_stackp, boot_stacklen, boot_priority,
      boot_threadf, NULL, "R2P_BOOT"
    );
    R2P_ASSERT(boot_threadp != NULL);
  }
#endif

  mgmt_threadp = Thread::create_static(
    mgmt_stackp, mgmt_stacklen, mgmt_priority,
    mgmt_threadf, NULL, "R2P_MGMT"
  );
  R2P_ASSERT(mgmt_threadp != NULL);

  // Wait until the info topic is fully initialized
  Thread::Priority oldprio = Thread::get_priority();
  Thread::set_priority(Thread::IDLE);
  SysLock::acquire();
  while (!mgmt_topic.has_local_publishers() || !mgmt_topic.has_local_subscribers()) {
    SysLock::release();
    Thread::sleep(Time::ms(500));
    SysLock::acquire();
  }
  SysLock::release();
  Thread::set_priority(oldprio);

#if R2P_USE_BOOTLOADER
  if (!is_bootloader_mode()) {
    // Launch all installed apps
    bool success; (void)success;
    success = Bootloader::launch_all();
    R2P_ASSERT(success);
  }
#endif
}


void Middleware::stop() {

  SysLock::acquire();
  if (!stopped) {
    stopped = true;
    SysLock::release();
  } else {
    SysLock::release();
    return;
  }

  // Stop all nodes
  SysLock::acquire();
  while (num_running_nodes > 0) {
    SysLock::release();
    for (StaticList<Node>::Iterator i = nodes.begin(); i != nodes.end(); ++i) {
        i->notify_stop();
    }
    Thread::sleep(Time::ms(500)); // TODO: Configure delay
    SysLock::acquire();
  }
  SysLock::release();
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

  lists_lock.acquire();
  Topic *topicp = touch_topic(namep, type_size);
  if (topicp == NULL) return false;
  pub.notify_advertised(*topicp);
  topicp->advertise(pub, publish_timeout);
  lists_lock.release();

  for (StaticList<Transport>::Iterator i = transports.begin();
       i != transports.end(); ++i) {
    i->notify_advertisement(*topicp);
  }

  return true;
}


bool Middleware::advertise(RemotePublisher &pub, const char *namep,
                           const Time &publish_timeout, size_t type_size) {

  lists_lock.acquire();
  Topic *topicp = touch_topic(namep, type_size);
  if (topicp == NULL) return false;
  pub.notify_advertised(*topicp);
  topicp->advertise(pub, publish_timeout);
  lists_lock.release();
/* FIXME: Is this needed?
  for (StaticList<Transport>::Iterator i = transports.begin();
      i != transports.end(); ++i) {
    i->notify_advertisement(*topicp);
  }
*/
  return true;
}


bool Middleware::subscribe(LocalSubscriber &sub, const char *namep,
                           Message msgpool_buf[], size_t msgpool_buflen,
                           size_t type_size) {

  lists_lock.acquire();
  Topic *topicp = touch_topic(namep, type_size);
  if (topicp == NULL) return false;
  topicp->extend_pool(msgpool_buf, msgpool_buflen);
  sub.notify_subscribed(*topicp);
  topicp->subscribe(sub, msgpool_buflen);
  lists_lock.release();

  for (StaticList<Transport>::Iterator i = transports.begin();
      i != transports.end(); ++i) {
    i->notify_subscription_request(*topicp);
  }

  return true;
}


bool Middleware::subscribe(RemoteSubscriber &sub, const char *namep,
                           Message msgpool_buf[], size_t msgpool_buflen,
                           size_t type_size) {

  (void) type_size; // FIXME

  lists_lock.acquire();
  Topic *topicp = find_topic(namep);
  R2P_ASSERT(topicp != NULL);
  topicp->extend_pool(msgpool_buf, msgpool_buflen);
  sub.notify_subscribed(*topicp);
  topicp->subscribe(sub, msgpool_buflen);
  lists_lock.release();

  return true;
}


void Middleware::confirm_stop(const Node &node) {

  SysLock::acquire();
  R2P_ASSERT(num_running_nodes > 0);
  R2P_ASSERT(nodes.contains_unsafe(node));
  --num_running_nodes;
  nodes.unlink_unsafe(node.by_middleware);
  SysLock::release();
}


Topic *Middleware::find_topic(const char *namep) {

  lists_lock.acquire();
  Topic *topicp = topics.find_first(Topic::has_name, namep);
  lists_lock.release();
  return topicp;
}


Node *Middleware::find_node(const char *namep) {

  lists_lock.acquire();
  Node *nodep = nodes.find_first(Node::has_name, namep);
  lists_lock.release();
  return nodep;
}


Topic *Middleware::touch_topic(const char *namep, size_t type_size) {

  lists_lock.acquire();

  // Check if there is a topic with the desired name
  Topic *topicp = find_topic(namep);
  if (topicp != NULL) {
    lists_lock.release();
    return topicp;
  }

  // Allocate a new topic
  topicp = new Topic(namep, type_size);
  if (topicp != NULL) {
    topics.link(topicp->by_middleware);
  }

  lists_lock.release();
  return topicp;
}


void Middleware::do_mgmt_thread() {

  MgmtMsg msgbuf[MGMT_BUFFER_LENGTH];
  MgmtMsg *msgqueue_buf[MGMT_BUFFER_LENGTH];
  Subscriber<MgmtMsg> sub(msgqueue_buf, MGMT_BUFFER_LENGTH);
  Publisher<MgmtMsg> pub;
  Node node("R2P_MGMT");

  node.advertise(pub, mgmt_topic.get_name(), Time::INFINITE);
  node.subscribe(sub, mgmt_topic.get_name(), msgbuf);

  MgmtMsg *msgp;
  Time deadline;
  do {
    if (node.spin(Time::ms(MGMT_TIMEOUT_MS))) {
      while (sub.fetch(msgp, deadline)) {
        switch (msgp->type) {
        case MgmtMsg::CMD_ADVERTISE: {
          Transport *transportp = msgp->pubsub.transportp;
          R2P_ASSERT(transportp != NULL);
          Topic *topicp = find_topic(msgp->pubsub.topic);
          sub.release(*msgp);
          if ((topicp != NULL) && topicp->has_local_subscribers()) {
            transportp->notify_subscription_request(*topicp);
          }
          break;
        }
        case MgmtMsg::CMD_SUBSCRIBE_REQUEST: {
          Transport *transportp = msgp->pubsub.transportp;
          R2P_ASSERT(transportp != NULL);
          size_t queue_length = msgp->pubsub.queue_length;
          Topic *topicp = find_topic(msgp->pubsub.topic);
          sub.release(*msgp);
          if (topicp != NULL && topicp->has_local_publishers()) { // TODO: will this prevent gw from proxiyng messages?
            transportp->subscribe_cb(*topicp, queue_length);
            while (!transportp->notify_subscription_response(*topicp)) {
              Thread::yield();
            }
          }
          break;
        }
        case MgmtMsg::CMD_SUBSCRIBE_RESPONSE: {
          Transport *transportp = msgp->pubsub.transportp;
          R2P_ASSERT(transportp != NULL);
          Topic *topicp = find_topic(msgp->pubsub.topic);
          if (topicp != NULL) {
            // TODO: copy to stack
            transportp->advertise_cb(*topicp, msgp->pubsub.raw_params);
          }
          sub.release(*msgp);
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
        case MgmtMsg::CMD_STOP: {
          if (0 == strncmp(module_namep, msgp->module.name,
                           NamingTraits<Middleware>::MAX_LENGTH)) {
            stop();
          }
          sub.release(*msgp);
          break;
        }
        case MgmtMsg::CMD_REBOOT: {
          if (0 == strncmp(module_namep, msgp->module.name,
                           NamingTraits<Middleware>::MAX_LENGTH)) {
            preload_bootloader_mode(false);
            reboot();
          }
          sub.release(*msgp);
          break;
        }
        case MgmtMsg::CMD_BOOTLOAD: {
          if (0 == strncmp(module_namep, msgp->module.name,
                           NamingTraits<Middleware>::MAX_LENGTH)) {
            preload_bootloader_mode(true);
            reboot();
          }
          sub.release(*msgp);
          break;
        }
        default: {
          sub.release(*msgp);
          break;
        }
        }
      }
    }

#if 0//XXX
    // TODO: Fine grained iterations by saving iterators; maybe in another function
    // Check for the next unadvertised topic
    if (topic_iter == topics.end()) {
      topics.restart(topic_iter);
    }
    if (topic_iter != topics.end()) {
      Time now = Time::now();
      if (now >= topic_lastiter_time + Time::ms(TOPIC_CHECK_TIMEOUT_MS)) {
        topic_lastiter_time = now; // TODO: Add random time
        SysLock::acquire();
        if (topic_iter->is_awaiting_advertisements()) {
          SysLock::release();
          for (StaticList<Transport>::Iterator i = transports.begin();
               i != transports.end(); ++i) {
            i->notify_subscription_request(*topic_iter);
          }
        } else {
          SysLock::release();
        }
        ++topic_iter;
      }
    }
#endif //0

    Thread::yield();
  } while (!instance.is_stopped());
}


Middleware::Middleware(const char *module_namep, const char *bootloader_namep)
:
  module_namep(module_namep),
  lists_lock(false),
  mgmt_topic("R2P", sizeof(MgmtMsg)),
  mgmt_stackp(NULL),
  mgmt_stacklen(0),
  mgmt_threadp(NULL),
  mgmt_priority(Thread::LOWEST),
#if R2P_USE_BOOTLOADER
  boot_topic(bootloader_namep, sizeof(BootMsg)),
  boot_stackp(NULL),
  boot_stacklen(0),
  boot_threadp(NULL),
  boot_priority(Thread::LOWEST),
#endif
  topic_iter(topics.end()),
  stopped(false),
  num_running_nodes(0)
{
	R2P_ASSERT(is_identifier(module_namep,
	                         NamingTraits<Middleware>::MAX_LENGTH));
	(void)bootloader_namep;
}


Thread::Return Middleware::mgmt_threadf(Thread::Argument) {

  instance.do_mgmt_thread();
  return Thread::OK;
}


#if R2P_USE_BOOTLOADER
Thread::Return Middleware::boot_threadf(Thread::Argument) {

  Flasher::Data *flash_page_bufp = new Flasher::Data[BOOT_PAGE_LENGTH];
  R2P_ASSERT(flash_page_bufp != NULL);

  BootMsg msgbuf[BOOT_BUFFER_LENGTH];
  BootMsg *msgqueue_buf[BOOT_BUFFER_LENGTH];
  Subscriber<BootMsg> sub(msgqueue_buf, BOOT_BUFFER_LENGTH);
  Publisher<BootMsg> pub;

  Node node("R2P_BOOT");
  { bool success; (void)success;
  success = node.advertise(pub, instance.boot_topic.get_name());
  R2P_ASSERT(success);
  success = node.subscribe(sub, instance.boot_topic.get_name(), msgbuf);
  R2P_ASSERT(success); }

  Bootloader bootloader(flash_page_bufp, pub, sub);
  bootloader.spin_loop();

  R2P_ASSERT(false);
  return Thread::OK;
}
#endif


} // namespace r2p
