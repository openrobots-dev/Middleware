#pragma once

#include <r2p/common.hpp>
#include <r2p/StaticList.hpp>
#include <r2p/Topic.hpp>
#include <r2p/Thread.hpp>
#include <r2p/MemoryPool.hpp>
#include <r2p/MgmtMsg.hpp>
#include <r2p/Publisher.hpp>
#include <r2p/Subscriber.hpp>
#include <r2p/Node.hpp>
#include <r2p/Bootloader.hpp>
#include <r2p/ReMutex.hpp>

namespace r2p {

#if !defined(R2P_ITERATE_PUBSUB) || defined(__DOXYGEN__)
#define R2P_ITERATE_PUBSUB      1
#endif


class Node;
class Transport;
class Time;
class Topic;
class LocalPublisher;
class LocalSubscriber;
class RemotePublisher;
class RemoteSubscriber;


class Middleware : private Uncopyable {
public:

  struct PubSubStep {
    PubSubStep  *nextp;
    Time        timestamp;
    Transport   *transportp;
    size_t      payload_size;
    char        topic[NamingTraits<Topic>::MAX_LENGTH];
    uint8_t     type;
  };

private:
  const char *const module_namep;
  StaticList<Node> nodes;
  StaticList<Topic> topics;
  StaticList<Transport> transports;
  ReMutex lists_lock;


  enum { MGMT_BUFFER_LENGTH = 10 };
  enum { MGMT_TIMEOUT_MS = 33 };
  Topic mgmt_topic;
  void *mgmt_stackp;
  size_t mgmt_stacklen;
  Thread *mgmt_threadp;
  Thread::Priority mgmt_priority;
  Node mgmt_node;
  Publisher<MgmtMsg> mgmt_pub;
  Subscriber<MgmtMsg> mgmt_sub;
  MgmtMsg *mgmt_queue_buf[MGMT_BUFFER_LENGTH];
  MgmtMsg mgmt_msg_buf[MGMT_BUFFER_LENGTH];

#if R2P_USE_BOOTLOADER
  enum { REBOOTED_MAGIC = 0xFEB007ED };
  enum { BOOT_MODE_MAGIC = 0x51B0077A };
  enum { BOOT_PAGE_LENGTH = 1 << 10 };
  enum { BOOT_BUFFER_LENGTH = 4 };
  Topic boot_topic;
  void *boot_stackp;
  size_t boot_stacklen;
  Thread *boot_threadp;
  Thread::Priority boot_priority;
#endif // R2P_USE_BOOTLOADER
#if R2P_USE_BRIDGE_MODE
  PubSubStep    *pubsub_stepsp;
  MemoryPool<PubSubStep> pubsub_pool;
#endif // R2P_USE_BRIDGE_MODE

#if R2P_ITERATE_PUBSUB
  enum { ITER_TIMEOUT_MS = 500 };
  StaticList<Node>::ConstIterator iter_nodes;
  StaticList<LocalPublisher>::ConstIterator iter_publishers;
  StaticList<LocalSubscriber>::ConstIterator iter_subscribers;
  Time iter_lasttime;
#endif

  bool stopped;
  size_t num_running_nodes;

public:
  static Middleware instance;
  static uint32_t rebooted_magic;
  static uint32_t boot_mode_magic;

public:
  const char *get_module_name() const;
  const StaticList<Node> &get_nodes() const;
  const StaticList<Topic> &get_topics() const;
  const StaticList<Transport> &get_transports() const;
  Topic &get_mgmt_topic();
  Topic &get_boot_topic();
  bool is_stopped() const;

  void initialize(void *mgmt_stackp, size_t mgmt_stacklen,
                  Thread::Priority mgmt_priority,
                  void *boot_stackp = NULL, size_t boot_stacklen = 0,
                  Thread::Priority boot_priority = Thread::LOWEST);
  void start();
  void stop();
  void reboot();
  void preload_bootloader_mode(bool enable);

  bool stop_remote(const char *namep);
  bool reboot_remote(const char *namep, bool bootload = false);

  void add(Node &node);
  void add(Transport &transport);
  void add(Topic &topic);

  bool advertise(LocalPublisher &pub, const char *namep,
                 const Time &publish_timeout, size_t type_size);
  bool advertise(RemotePublisher &pub, const char *namep,
                 const Time &publish_timeout, size_t type_size);
  bool subscribe(LocalSubscriber &sub, const char *namep,
                 Message msgpool_buf[], size_t msgpool_buflen,
                 size_t type_size);
  bool subscribe(RemoteSubscriber &sub, const char *namep,
                 Message msgpool_buf[], size_t msgpool_buflen,
                 size_t type_size);

  void confirm_stop(const Node &node);

  Topic *find_topic(const char *namep);
  Node *find_node(const char *namep);

private:
  Topic *touch_topic(const char *namep, size_t type_size);

private:
  Middleware(const char *module_namep, const char *bootloader_namep,
             PubSubStep pubsub_buf[] = NULL, size_t pubsub_length = 0);

private:
  static Thread::Return mgmt_threadf(Thread::Argument);
  void do_mgmt_thread();
  void do_cmd_advertise(const MgmtMsg &msg);
  void do_cmd_subscribe_request(const MgmtMsg &msg);
  void do_cmd_subscribe_response(const MgmtMsg &msg);
#if R2P_USE_BRIDGE_MODE
  PubSubStep *alloc_pubsub_step();
#endif

#if R2P_USE_BOOTLOADER
  static Thread::Return boot_threadf(Thread::Argument);
  void do_boot_thread();
#endif

public:
  static bool has_rebooted();
  static bool is_bootloader_mode();
};


inline
const char *Middleware::get_module_name() const {

  return module_namep;
}


inline
const StaticList<Node> &Middleware::get_nodes() const {

  return nodes;
}


inline
const StaticList<Topic> &Middleware::get_topics() const {

  return topics;
}


inline
const StaticList<Transport> &Middleware::get_transports() const {

  return transports;
}


inline
Topic &Middleware::get_mgmt_topic() {

  return mgmt_topic;
}


inline
bool Middleware::is_stopped() const {

  return stopped;
}


#if R2P_USE_BOOTLOADER
inline
Topic &Middleware::get_boot_topic() {

  return boot_topic;
}


inline
void Middleware::preload_bootloader_mode(bool enable) {

  boot_mode_magic = enable ? BOOT_MODE_MAGIC : ~BOOT_MODE_MAGIC;
}


inline
bool Middleware::has_rebooted() {

  return rebooted_magic == REBOOTED_MAGIC;
}


inline
bool Middleware::is_bootloader_mode() {

  return boot_mode_magic == BOOT_MODE_MAGIC;
}
#endif

inline
bool ok() {

  SysLock::acquire();
  bool alive = !Middleware::instance.is_stopped();
  SysLock::release();
  return alive;
}


} // namespace r2p
