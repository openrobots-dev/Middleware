
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
#if R2P_USE_BOOTLOADER
  this->boot_stackp = boot_stackp;
  this->boot_stacklen = boot_stacklen;
  this->boot_priority = boot_priority;
#endif

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
  SysLock::acquire();
  while (!mgmt_topic.has_local_publishers() || !mgmt_topic.has_local_subscribers()) {
    SysLock::release();
    Thread::sleep(Time::ms(500)); // TODO: configure
    SysLock::acquire();
  }
  SysLock::release();

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

  { SysLock::Scope lock;
  if (!stopped) stopped = true;
  else return; }

  // Stop all nodes
  for (;;) {
    for (StaticList<Node>::Iterator i = nodes.begin(); i != nodes.end(); ++i) {
        i->notify_stop();
    }
    Thread::sleep(Time::ms(500)); // TODO: Configure delay
    { SysLock::Scope lock;
    if (num_running_nodes == 0) break; }
  }
}


bool Middleware::stop_remote(const char *namep) {

  MgmtMsg *msgp;
  if (mgmt_pub.alloc(msgp)) {
    msgp->type = MgmtMsg::STOP;
    strncpy(msgp->module.name, namep, NamingTraits<Middleware>::MAX_LENGTH);
    return mgmt_pub.publish(*msgp);
  }
  return false;
}


bool Middleware::reboot_remote(const char *namep, bool bootload) {

  MgmtMsg *msgp;
  if (mgmt_pub.alloc(msgp)) {
    msgp->type = bootload ? MgmtMsg::BOOTLOAD : MgmtMsg::REBOOT;
    strncpy(msgp->module.name, namep, NamingTraits<Middleware>::MAX_LENGTH);
    return mgmt_pub.publish(*msgp);
  }
  return false;
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

  { SysLock::Scope lock;
  if (mgmt_pub.get_topic() == NULL) return true; }

  MgmtMsg *msgp;
  if (mgmt_pub.alloc(msgp)) {
    msgp->type = MgmtMsg::ADVERTISE;
    strncpy(msgp->pubsub.topic, topicp->get_name(),
            NamingTraits<Topic>::MAX_LENGTH);
    msgp->pubsub.payload_size = topicp->get_payload_size();
    mgmt_pub.publish_remotely(*msgp);
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
    { SysLock::Scope lock;
    if (mgmt_pub.get_topic() == NULL) return true; }

    MgmtMsg *msgp;
    if (mgmt_pub.alloc(msgp)) {
      msgp->type = MgmtMsg::SUBSCRIBE_REQUEST;
      strncpy(msgp->pubsub.topic, topicp->get_name(),
              NamingTraits<Topic>::MAX_LENGTH);
      msgp->pubsub.payload_size = topicp->get_payload_size();
      msgp->pubsub.queue_length = msgpool_buflen;
      mgmt_pub.publish_remotely(*msgp);
    }
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

  SysLock::acquire();
  mgmt_node.set_enabled(true);
  SysLock::release();
  mgmt_node.advertise(mgmt_pub, mgmt_topic.get_name(), Time::INFINITE);
  mgmt_node.subscribe(mgmt_sub, mgmt_topic.get_name(), mgmt_msg_buf);

  // Tell it is alive
  MgmtMsg *msgp;
  if (mgmt_pub.alloc(msgp)) {
    msgp->type = MgmtMsg::ALIVE;
    strncpy(msgp->module.name, module_namep,
            NamingTraits<Middleware>::MAX_LENGTH);
    SysLock::acquire();
    msgp->module.flags.stopped = is_stopped() ? 1 : 0;
#if R2P_USE_BOOTLOADER
    msgp->module.flags.rebooted = has_rebooted() ? 1 : 0;
    msgp->module.flags.boot_mode = is_bootloader_mode() ? 1 : 0;
#endif
    SysLock::release();
    mgmt_pub.publish(*msgp);
  }

  // Message dispatcher
  for (;;) {
    if (mgmt_node.spin(Time::ms(MGMT_TIMEOUT_MS))) {
      Time deadline;
      while (mgmt_sub.fetch(msgp, deadline)) {
        switch (msgp->type) {
        case MgmtMsg::ADVERTISE: {
          do_cmd_advertise(*msgp);
          mgmt_sub.release(*msgp);
          break;
        }
        case MgmtMsg::SUBSCRIBE_REQUEST: {
          do_cmd_subscribe_request(*msgp);
          mgmt_sub.release(*msgp);
          break;
        }
        case MgmtMsg::SUBSCRIBE_RESPONSE: {
          do_cmd_subscribe_response(*msgp);
          mgmt_sub.release(*msgp);
          break;
        }
        case MgmtMsg::STOP: {
          if (0 == strncmp(module_namep, msgp->module.name,
                           NamingTraits<Middleware>::MAX_LENGTH)) {
            stop();
          }
          mgmt_sub.release(*msgp);
          break;
        }
        case MgmtMsg::REBOOT: {
          if (0 == strncmp(module_namep, msgp->module.name,
                           NamingTraits<Middleware>::MAX_LENGTH)) {
#if R2P_USE_BOOTLOADER
            preload_bootloader_mode(false);
#endif
            reboot();
          }
          mgmt_sub.release(*msgp);
          break;
        }
#if R2P_USE_BOOTLOADER
        case MgmtMsg::BOOTLOAD: {
          if (0 == strncmp(module_namep, msgp->module.name,
                           NamingTraits<Middleware>::MAX_LENGTH)) {
            preload_bootloader_mode(true);
            reboot();
          }
          mgmt_sub.release(*msgp);
          break;
        }
#endif
        default: {
          mgmt_sub.release(*msgp);
          break;
        }
        }
      }
    }
#if R2P_ITERATE_PUBSUB
    // Iterate publishers and subscribers
    else if (Time::now() - iter_lasttime >= Time::ms(ITER_TIMEOUT_MS)) {
      iter_lasttime = Time::now();
      if (!iter_nodes.is_valid()) {
        // Restart nodes iteration
        nodes.restart(iter_nodes);
        if (iter_nodes.is_valid()) {
          iter_nodes->get_publishers().restart(iter_publishers);
          iter_nodes->get_subscribers().restart(iter_subscribers);
        }

        // Tell it is alive
        if (mgmt_pub.alloc(msgp)) {
          msgp->type = MgmtMsg::ALIVE;
          strncpy(msgp->module.name, module_namep,
                  NamingTraits<Middleware>::MAX_LENGTH);
          SysLock::acquire();
          msgp->module.flags.stopped = is_stopped() ? 1 : 0;
#if R2P_USE_BOOTLOADER
          msgp->module.flags.rebooted = has_rebooted() ? 1 : 0;
          msgp->module.flags.boot_mode = is_bootloader_mode() ? 1 : 0;
#endif
          SysLock::release();
          mgmt_pub.publish(*msgp);
        }
      }
      else if (iter_publishers.is_valid()) {
        // Advertise next publisher
        if (mgmt_pub.alloc(msgp)) {
          Message::reset_payload(*msgp);
          msgp->type = MgmtMsg::ADVERTISE;
          lists_lock.acquire();
          const Topic &topic = *iter_publishers->get_topic();
          msgp->pubsub.payload_size = topic.get_payload_size();
          strncpy(msgp->pubsub.topic, topic.get_name(),
                  NamingTraits<Topic>::MAX_LENGTH);
          msgp->pubsub.queue_length = topic.get_max_queue_length();
          lists_lock.release();
          if (mgmt_pub.publish_remotely(*msgp)) {
            ++iter_publishers;
          }
        }
      }
      else if (iter_subscribers.is_valid()) {
        // Subscribe next subscriber
        if (mgmt_pub.alloc(msgp)) {
          Message::reset_payload(*msgp);
          msgp->type = MgmtMsg::SUBSCRIBE_REQUEST;
          SysLock::acquire();
          const Topic &topic = *iter_subscribers->get_topic();
          msgp->pubsub.payload_size = topic.get_payload_size();
          strncpy(msgp->pubsub.topic, topic.get_name(),
                  NamingTraits<Topic>::MAX_LENGTH);
          msgp->pubsub.queue_length = topic.get_max_queue_length();
          SysLock::release();
          if (mgmt_pub.publish_remotely(*msgp)) {
            ++iter_subscribers;
          }
        }
      }
      else {
        ++iter_nodes;
        if (iter_nodes.is_valid()) {
          iter_nodes->get_publishers().restart(iter_publishers);
          iter_nodes->get_subscribers().restart(iter_subscribers);
        }
      }
    }
#endif // R2P_ITERATE_PUBSUB
  }
}


void Middleware::do_cmd_advertise(const MgmtMsg &msg) {

  Topic *topicp = find_topic(msg.pubsub.topic);
#if R2P_USE_BRIDGE_MODE
  R2P_ASSERT(msg.get_source() != NULL);
  if (topicp != NULL) {
#else // R2P_USE_BRIDGE_MODE
  R2P_ASSERT(transports.count() == 1);
  if (topicp != NULL && topicp->has_local_subscribers()) {
#endif // R2P_USE_BRIDGE_MODE
    { SysLock::Scope lock;
    if (mgmt_pub.get_topic() == NULL) return; }

    MgmtMsg *msgp;
    if (mgmt_pub.alloc(msgp)) {
      msgp->type = MgmtMsg::SUBSCRIBE_REQUEST;
      strncpy(msgp->pubsub.topic, topicp->get_name(),
              NamingTraits<Topic>::MAX_LENGTH);
      msgp->pubsub.payload_size = topicp->get_payload_size();
      msgp->pubsub.queue_length = topicp->get_max_queue_length();
      mgmt_pub.publish_remotely(*msgp);
    }
  }
#if R2P_USE_BRIDGE_MODE
  else {
    PubSubStep *curp;
    for (curp = pubsub_stepsp; curp != NULL; curp = curp->nextp) {
      if (0 == strncmp(curp->topic, msg.pubsub.topic,
                       NamingTraits<Topic>::MAX_LENGTH) &&
          curp->type == MgmtMsg::ADVERTISE) {
        // Advertisement already cached
        curp->timestamp = Time::now();
        return;
      }
    }
    curp = alloc_pubsub_step();
    memset(curp, 0, sizeof(PubSubStep));
    curp->nextp = pubsub_stepsp;
    curp->timestamp = Time::now();
    curp->transportp = msg.get_source();
    curp->payload_size = msg.pubsub.payload_size;
    strncpy(curp->topic, msg.pubsub.topic, NamingTraits<Topic>::MAX_LENGTH);
    curp->type = MgmtMsg::ADVERTISE;
    pubsub_stepsp = curp;
  }
#endif // R2P_USE_BRIDGE_MODE
}


void Middleware::do_cmd_subscribe_request(const MgmtMsg &msg) {

  Topic *topicp = find_topic(msg.pubsub.topic);
#if R2P_USE_BRIDGE_MODE
  R2P_ASSERT(msg.get_source() != NULL);
  if (topicp != NULL) {
    msg.get_source()->subscribe_cb(*topicp, msg.pubsub.queue_length);
#else // R2P_USE_BRIDGE_MODE
  R2P_ASSERT(transports.count() == 1);
  if (topicp != NULL && topicp->has_local_publishers()) {
    transports.begin()->subscribe_cb(*topicp, msg.pubsub.queue_length);
#endif // R2P_USE_BRIDGE_MODE
    { SysLock::Scope lock;
    if (mgmt_pub.get_topic() == NULL) return; }

    MgmtMsg *msgp;
    if (mgmt_pub.alloc(msgp)) {
      msgp->type = MgmtMsg::SUBSCRIBE_RESPONSE;
      strncpy(msgp->pubsub.topic, topicp->get_name(),
              NamingTraits<Topic>::MAX_LENGTH);
      msgp->pubsub.payload_size = topicp->get_payload_size();
      mgmt_pub.publish_remotely(*msgp);
    }
  }
#if R2P_USE_BRIDGE_MODE
  else {
    PubSubStep *curp, *prevp = NULL;
    for (curp = pubsub_stepsp; curp != NULL; prevp = curp, curp = curp->nextp) {
      if (0 == strncmp(curp->topic, msg.pubsub.topic,
                       NamingTraits<Topic>::MAX_LENGTH)) {
        if (curp->type == MgmtMsg::SUBSCRIBE_REQUEST) {
          // Subscription request already cached
          curp->timestamp = Time::now();
          return;
        } else if (curp->type == MgmtMsg::ADVERTISE) {
          // Subscription request matching advertisement
          if (prevp != NULL) {
            prevp->nextp = curp->nextp;
          }
          char *namep = new char[NamingTraits<Topic>::MAX_LENGTH];
          R2P_ASSERT(namep != NULL);
          strncpy(namep, curp->topic, NamingTraits<Topic>::MAX_LENGTH);
          topicp = touch_topic(namep,
                               Message::get_type_size(curp->payload_size));
          R2P_ASSERT(topicp != NULL);
          pubsub_pool.free(curp);
          curp = NULL;
          break;
        }
      }
    }
    if (curp == NULL) {
      curp = alloc_pubsub_step();
      memset(curp, 0, sizeof(PubSubStep));
      curp->nextp = pubsub_stepsp;
      curp->timestamp = Time::now();
      curp->transportp = msg.get_source();
      curp->payload_size = msg.pubsub.payload_size;
      strncpy(curp->topic, msg.pubsub.topic, NamingTraits<Topic>::MAX_LENGTH);
      curp->type = MgmtMsg::SUBSCRIBE_REQUEST;
      pubsub_stepsp = curp;
    }
  }
#endif // R2P_USE_BRIDGE_MODE
}


void Middleware::do_cmd_subscribe_response(const MgmtMsg &msg) {

#if R2P_USE_BRIDGE_MODE
  R2P_ASSERT(msg.get_source() != NULL);

  Topic *topicp = find_topic(msg.pubsub.topic);
  if (topicp != NULL) {
    msg.get_source()->advertise_cb(*topicp, msg.pubsub.raw_params);
  }
  else {
    PubSubStep *curp, *prevp = NULL;
    for (curp = pubsub_stepsp; curp != NULL; prevp = curp, curp = curp->nextp) {
      if (0 == strncmp(curp->topic, msg.pubsub.topic,
                       NamingTraits<Topic>::MAX_LENGTH) &&
          curp->type == MgmtMsg::SUBSCRIBE_REQUEST) {
        // Subscription response matching request
        if (prevp != NULL) {
          prevp->nextp = curp->nextp;
        }
        char *namep = new char[NamingTraits<Topic>::MAX_LENGTH];
        R2P_ASSERT(namep != NULL);
        strncpy(namep, curp->topic, NamingTraits<Topic>::MAX_LENGTH);
        topicp = touch_topic(namep, Message::get_type_size(curp->payload_size));
        R2P_ASSERT(topicp != NULL);
        pubsub_pool.free(curp);
        curp = NULL;
        break;
      }
    }
  }
#else // R2P_USE_BRIDGE_MODE
  R2P_ASSERT(transports.count() == 1);

  Topic *topicp = find_topic(msg.pubsub.topic);
  if (topicp != NULL) {
    transports.begin()->advertise_cb(*topicp, msg.pubsub.raw_params);
  }
#endif // R2P_USE_BRIDGE_MODE
}


#if R2P_USE_BRIDGE_MODE

Middleware::PubSubStep *Middleware::alloc_pubsub_step() {

  PubSubStep *stepp = pubsub_pool.alloc();
  if (stepp != NULL) {
    return stepp;
  } else {
    Time now = Time::now();
    PubSubStep *oldestp = pubsub_stepsp;
    for (stepp = pubsub_stepsp; stepp != NULL; stepp = stepp->nextp) {
      if ((now - stepp->timestamp) > (now - oldestp->timestamp)) {
        oldestp = stepp;
      }
    }
    return oldestp;
  }
}

#endif // R2P_USE_BRIDGE_MODE


Middleware::Middleware(const char *module_namep, const char *bootloader_namep,
                       PubSubStep pubsub_buf[], size_t pubsub_length)
:
  module_namep(module_namep),
  lists_lock(false),
  mgmt_topic("R2P", sizeof(MgmtMsg)),
  mgmt_stackp(NULL),
  mgmt_stacklen(0),
  mgmt_threadp(NULL),
  mgmt_priority(Thread::LOWEST),
  mgmt_node("R2P_MGMT", false),
  mgmt_pub(),
  mgmt_sub(mgmt_queue_buf, MGMT_BUFFER_LENGTH, NULL),
#if R2P_USE_BOOTLOADER
  boot_topic(bootloader_namep, sizeof(BootMsg)),
  boot_stackp(NULL),
  boot_stacklen(0),
  boot_threadp(NULL),
  boot_priority(Thread::LOWEST),
#endif
#if R2P_USE_BRIDGE_MODE
  pubsub_stepsp(NULL),
  pubsub_pool(pubsub_buf, pubsub_length),
#endif
  stopped(false),
  num_running_nodes(0)
{
	R2P_ASSERT(is_identifier(module_namep,
	                         NamingTraits<Middleware>::MAX_LENGTH));
	(void)bootloader_namep;
	(void)pubsub_buf;
	(void)pubsub_length;
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
