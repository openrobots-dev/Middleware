
#include <r2p/transport/DebugTransport.hpp>
#include <r2p/transport/DebugPublisher.hpp>
#include <r2p/transport/DebugSubscriber.hpp>
#include <r2p/Middleware.hpp>
#include <r2p/ScopedLock.hpp>
#include <r2p/Checksummer.hpp>

#include <cstring>
#include <locale>

namespace r2p {


bool DebugTransport::skip_after_char(char c, systime_t timeout) {

  register msg_t value;
  do {
    value = chnGetTimeout(channelp, timeout);
    if ((value & ~0xFF) != 0) return false;
  } while (static_cast<char>(value) != c);
  return true;
}


bool DebugTransport::recv_byte(uint8_t &byte, systime_t timeout) {

  msg_t c;

  c = chnGetTimeout(channelp, timeout);
  if        ((c >= '0' && c <= '9')) {
    byte = (static_cast<uint8_t>(c) - '0') << 4;
  } else if ((c >= 'A' && c <= 'F')) {
    byte = (static_cast<uint8_t>(c) + (10 - 'A')) << 4;
  } else if ((c >= 'a' && c <= 'f')) {
    byte = (static_cast<uint8_t>(c) + (10 - 'a')) << 4;
  } else {
    return false;
  }

  c = chnGetTimeout(channelp, timeout);
  if        ((c >= '0' && c <= '9')) {
    byte |= static_cast<uint8_t>(c) - '0';
  } else if ((c >= 'A' && c <= 'F')) {
    byte |= static_cast<uint8_t>(c) + (10 - 'A');
  } else if ((c >= 'a' && c <= 'f')) {
    byte |= static_cast<uint8_t>(c) + (10 - 'a');
  } else {
    return false;
  }

  return true;
}


bool DebugTransport::send_chunk(const void *chunkp, size_t length,
                                systime_t timeout) {

  const uint8_t *p = reinterpret_cast<const uint8_t *>(chunkp);
  while (length-- > 0) {
    if (!send_byte(*p++, timeout)) return false;
  }
  return true;
}


bool DebugTransport::recv_chunk(void *chunkp, size_t length,
                                systime_t timeout) {

  uint8_t *p = reinterpret_cast<uint8_t *>(chunkp);
  while (length-- > 0) {
    if (!recv_byte(*p++, timeout)) return false;
  }
  return true;
}


bool DebugTransport::send_chunk_rev(const void *chunkp, size_t length,
                                    systime_t timeout) {

  const uint8_t *p = reinterpret_cast<const uint8_t *>(chunkp) + length - 1;
  while (length-- > 0) {
    if (!send_byte(*p--, timeout)) return false;
  }
  return true;
}


bool DebugTransport::recv_chunk_rev(void *chunkp, size_t length,
                                    systime_t timeout) {

  uint8_t *p = reinterpret_cast<uint8_t *>(chunkp) + length - 1;
  while (length-- > 0) {
    if (!recv_byte(*p--, timeout)) return false;
  }
  return true;
}


bool DebugTransport::send_string(const char *stringp, size_t length,
                                 systime_t timeout) {

  for (; length-- > 0; ++stringp) {
    if (!send_char(*stringp, timeout)) return false;
  }
  return true;
}


bool DebugTransport::recv_string(char *stringp, size_t length,
                                 systime_t timeout) {

  for (; length-- > 0; ++stringp) {
    if (!recv_char(*stringp, timeout)) return false;
  }
  return true;
}


bool DebugTransport::send_msg(const Message &msg, size_t msg_size,
                              const char *topicp, const Time &deadline) {

  Checksummer cs;

  // Send the deadline
  if (!send_char('@')) return false;
  if (!send_value(deadline.raw)) return false;
  if (!send_char(':')) return false;
  cs.add(deadline.raw);

  // Send the topic name
  uint8_t namelen = static_cast<uint8_t>(
    strnlen(topicp, NamingTraits<Topic>::MAX_LENGTH)
  );
  if (!send_value(namelen)) return false;
  if (!send_string(topicp, namelen)) return false;
  cs.add(namelen);
  cs.add(topicp, namelen);

  // Send the payload length and data
  if (!send_char(':')) return false;
  namelen = static_cast<uint8_t>(msg_size);
  if (!send_value(namelen)) return false;
  if (!send_chunk(msg.get_raw_data(), msg_size)) return false;
  cs.add(namelen);
  cs.add(msg.get_raw_data(), msg_size);

  // Send the checksum
  if (!send_char(':')) return false;
  if (!send_value(cs.compute_checksum())) return false;

  // End of packet
  if (!send_char('\r')) return false;
  if (!send_char('\n')) return false;
  return true;
}


bool DebugTransport::send_pubsub_msg(const Topic &topic,
                                     MgmtMsg::TypeEnum type) {

  Checksummer cs;

  // Send the deadline
  if (!send_char('@')) return false;
  Time now = Time::now();
  if (!send_value(now.raw)) return false;
  if (!send_char(':')) return false;
  cs.add(now.raw);

  // Send the management topic name (null string)
  if (!send_value(static_cast<uint8_t>(0))) return false;
  if (!send_char(':')) return false;

  // Discriminate between publisher and subscriber
  switch (type) {
  case MgmtMsg::CMD_ADVERTISE: {
    if (!send_char('p')) return false;
    cs.add('p');
    break;
  }
  case MgmtMsg::CMD_SUBSCRIBE_REQUEST: {
    if (!send_char('s')) return false;
    SysLock::acquire();
    uint8_t length = static_cast<uint8_t>(topic.get_max_queue_length());
    SysLock::release();
    if (!send_value(length)) return false;
    cs.add('s');
    cs.add(length);
    break;
  }
  case MgmtMsg::CMD_SUBSCRIBE_RESPONSE: {
    if (!send_char('e')) return false;
    cs.add('e');
    break;
  }
  default: {
    R2P_ASSERT(false && "unreachable");
    break;
  }
  }
  if (!send_char(':')) return false;

  // Send the module and topic names
  const char *namep = Middleware::instance.get_module_name();
  uint8_t namelen = static_cast<uint8_t>(
    strnlen(namep, NamingTraits<Middleware>::MAX_LENGTH)
  );
  if (!send_value(namelen)) return false;
  if (!send_string(namep, namelen)) return false;
  if (!send_char(':')) return false;
  cs.add(namelen);
  cs.add(namep, namelen);

  namep = topic.get_name();
  namelen = static_cast<uint8_t>(
    strnlen(namep, NamingTraits<Topic>::MAX_LENGTH)
  );
  if (!send_value(namelen)) return false;
  if (!send_string(namep, namelen)) return false;
  cs.add(namelen);
  cs.add(namep, namelen);

  // Send the checksum
  if (!send_char(':')) return false;
  if (!send_value(cs.compute_checksum())) return false;

  // End of packet
  if (!send_char('\r')) return false;
  if (!send_char('\n')) return false;
  return true;
}


bool DebugTransport::send_stop_msg() {

  Checksummer cs;

  // Send the deadline
  if (!send_char('@')) return false;
  Time now = Time::now();
  if (!send_value(now.raw)) return false;
  if (!send_char(':')) return false;
  cs.add(now.raw);

  // Send the management topic name (null string)
  if (!send_value(static_cast<uint8_t>(0))) return false;
  if (!send_char(':')) return false;

  // Stop command
  if (!send_char('t')) return false;
  cs.add('t');

  // Send the checksum
  if (!send_char(':')) return false;
  if (!send_value(cs.compute_checksum())) return false;

  // End of packet
  if (!send_char('\r')) return false;
  if (!send_char('\n')) return false;
  return true;
}


bool DebugTransport::send_reboot_msg() {

  Checksummer cs;

  // Send the deadline
  if (!send_char('@')) return false;
  Time now = Time::now();
  if (!send_value(now.raw)) return false;
  if (!send_char(':')) return false;
  cs.add(now.raw);

  // Send the management topic name (null string)
  if (!send_value(static_cast<uint8_t>(0))) return false;
  if (!send_char(':')) return false;

  // Reboot command
  if (!send_char('r')) return false;
  cs.add('r');

  // Send the checksum
  if (!send_char(':')) return false;
  if (!send_value(cs.compute_checksum())) return false;

  // End of packet
  if (!send_char('\r')) return false;
  if (!send_char('\n')) return false;
  return true;
}


bool DebugTransport::send_advertisement(const Topic &topic) {

  ScopedLock<Mutex> lock(send_lock);
  return send_pubsub_msg(topic, MgmtMsg::CMD_ADVERTISE);
}


bool DebugTransport::send_subscription_request(const Topic &topic) {

  ScopedLock<Mutex> lock(send_lock);
  return send_pubsub_msg(topic, MgmtMsg::CMD_SUBSCRIBE_REQUEST);
}


bool DebugTransport::send_subscription_response(const Topic &topic) {

  ScopedLock<Mutex> lock(send_lock);
  return send_pubsub_msg(topic, MgmtMsg::CMD_SUBSCRIBE_RESPONSE);
}


bool DebugTransport::send_stop() {

  ScopedLock<Mutex> lock(send_lock);
  return send_stop_msg();
}


bool DebugTransport::send_reboot() {

  ScopedLock<Mutex> lock(send_lock);
  return send_reboot_msg();
}


void DebugTransport::initialize(void *rx_stackp, size_t rx_stacklen,
                                Thread::Priority rx_priority,
                                void *tx_stackp, size_t tx_stacklen,
                                Thread::Priority tx_priority) {

  subp_sem.initialize();
  send_lock.initialize();

  // Create the transmission pump thread
  tx_threadp = Thread::create_static(tx_stackp, tx_stacklen, tx_priority,
                                     tx_threadf, this, "DEBUG_TX");
  R2P_ASSERT(tx_threadp != NULL);

  // Create the reception pump thread
  rx_threadp = Thread::create_static(rx_stackp, rx_stacklen, rx_priority,
                                     rx_threadf, this, "DEBUG_RX" );
  R2P_ASSERT(rx_threadp != NULL);

  // Clear any previous crap caused by serial port initialization
  bool success; (void)success;
  send_lock.acquire();
  success = send_chunk("\r\n\r\n\r\n", 6);
  R2P_ASSERT(success);
  send_lock.release();

  // Register remote publisher and subscriber for the management thread
  success = advertise(mgmt_rpub, "R2P", Time::INFINITE, sizeof(MgmtMsg));
  R2P_ASSERT(success);
  success = subscribe(mgmt_rsub, "R2P", mgmt_msgbuf, MGMT_BUFFER_LENGTH,
                      sizeof(MgmtMsg));
  R2P_ASSERT(success);

  Middleware::instance.add(*this);
}


bool DebugTransport::spin_tx() {

  Message *msgp;
  Time timestamp;
  const BaseSubscriberQueue::Link *subp_linkp;

  SysLock::acquire();
  subp_sem.wait_unsafe();
  subp_queue.peek_unsafe(subp_linkp);
  R2P_ASSERT(subp_linkp->itemp != NULL);
  DebugSubscriber &sub = static_cast<DebugSubscriber &>(*subp_linkp->itemp);
  size_t queue_length = sub.get_queue_count();
  R2P_ASSERT(queue_length > 0);
  SysLock::release();

  while (queue_length-- > 0) {
    SysLock::acquire();
    sub.fetch_unsafe(msgp, timestamp);
    if (queue_length == 0) {
      subp_queue.skip_unsafe();
      if (sub.get_queue_count() > 0) {
        notify_first_sub_unsafe(sub);
      }
    }
    SysLock::release();

    const Topic &topic = *sub.get_topic();
    send_lock.acquire();
    if (!send_msg(*msgp, topic.get_size(), topic.get_name(),
                  timestamp + topic.get_publish_timeout())) {
      send_char('\r');
      while (!send_char('\n')) {
        Thread::yield();
      }
      send_lock.release();
      sub.release(*msgp);
      return false;
    }
    send_lock.release();
    sub.release(*msgp);
  }
  return true;
}


bool DebugTransport::spin_rx() {

  Checksummer cs;

  // Skip the deadline
  if (!skip_after_char('@')) return false;
  Thread::sleep(Time::ms(100)); // XXX XXX XXX XXX XXX XXX XXX XXX XXX XXX XXX
  { Time deadline;
  if (!recv_value(deadline.raw)) return false;
  cs.add(deadline.raw); }

  // Receive the topic name length and data
  if (!expect_char(':')) return false;
  uint8_t length;
  if (!recv_value(length)) return false;
  if (length > NamingTraits<Topic>::MAX_LENGTH) return false;
  if (!recv_string(namebufp, length)) return false;
  if (!expect_char(':')) return false;

  // Check if it is a management message
  if (length > 0) { // Normal message
    // Check if the topic is known
    Topic *topicp = Middleware::instance.find_topic(namebufp);
    if (topicp == NULL) return false;
    RemotePublisher *pubp;
    pubp = publishers.find_first(BasePublisher::has_topic, topicp->get_name());
    if (pubp == NULL) return false;
    cs.add(length);
    cs.add(namebufp, length);

    // Get the payload length
    if (!recv_value(length)) return false;
    if (length != topicp->get_size()) return false;
    cs.add(length);

    // Get the payload data
    Message *msgp;
    if (!pubp->alloc(msgp)) return false;
    if (!recv_chunk(const_cast<uint8_t *>(msgp->get_raw_data()), length)) {
      topicp->free(*msgp);
      return false;
    }
    cs.add(msgp->get_raw_data(), length);

    // Get the checksum
    if (!expect_char(':')) return false;
    if (!recv_value(length)) return false;
    if (length != cs.compute_checksum()) return false;

    // Forward the message locally
    return pubp->publish_locally(*msgp);
  }
  else { // Management message
    // Read the management message type character
    char typechar;
    if (!recv_char(typechar)) return false;
    typechar = static_cast<char>(tolower(typechar));
    cs.add(typechar);

    switch (typechar) {
    case 'p':
    case 's':
    case 'e': {
      // Get the module name (ignored)
      if (!expect_char(':')) return false;
      if (!recv_value(length)) return false;
      if (length < 1 || length > NamingTraits<Middleware>::MAX_LENGTH) {
        return false;
      }
      if (!recv_string(namebufp, length)) return false;
      cs.add(length);
      cs.add(namebufp, length);

      // Get the topic name
      if (!expect_char(':')) return false;
      if (!recv_value(length)) return false;
      if (length < 1 || length > NamingTraits<Topic>::MAX_LENGTH) {
        return false;
      }
      if (!recv_string(namebufp, length)) return false;
      cs.add(length);
      cs.add(namebufp, length);

      MgmtMsg *msgp;
      if (!mgmt_rpub.alloc(reinterpret_cast<Message *&>(msgp))) return false;
      Message::clean(*msgp);
      msgp->pubsub.transportp = this;
      strncpy(msgp->pubsub.topic, namebufp, NamingTraits<Topic>::MAX_LENGTH);

      switch (typechar) {
      case 'p': {
        msgp->type = MgmtMsg::CMD_ADVERTISE;
        break;
      }
      case 's': {
        // Get the queue length
        if (!recv_value(length) || length == 0) {
          mgmt_rsub.release(*msgp);
          return false;
        }
        cs.add(length);

        msgp->type = MgmtMsg::CMD_SUBSCRIBE_REQUEST;
        msgp->pubsub.queue_length = length;
        break;
      }
      case 'e': {
        msgp->type = MgmtMsg::CMD_SUBSCRIBE_RESPONSE;
        break;
      }
      }

      // Get the checksum
      if (!expect_char(':') || !recv_value(length) ||
          length != cs.compute_checksum()) {
        mgmt_rsub.release(*msgp);
        return false;
      }
      return mgmt_rpub.publish_locally(*msgp);
    }
    case 't': {
      // Get the checksum
      if (!expect_char(':')) return false;
      if (!recv_value(length)) return false;
      if (length != cs.compute_checksum()) return false;

      Middleware::instance.stop();
      return true;
    }
    case 'r': {
      // Get the checksum
      if (!expect_char(':')) return false;
      if (!recv_value(length)) return false;
      if (length != cs.compute_checksum()) return false;

      Middleware::instance.reboot();
      return true;
    }
    default: return false;
    }
  }
}


Thread::Return DebugTransport::rx_threadf(Thread::Argument arg) {

  R2P_ASSERT(arg != static_cast<Thread::Argument>(NULL));

  // Reception pump
  for (;;) {
    register bool ok;
    ok = reinterpret_cast<DebugTransport *>(arg)->spin_rx();
    (void)ok;
  }
  return static_cast<Thread::Return>(0);
}


Thread::Return DebugTransport::tx_threadf(Thread::Argument arg) {

  R2P_ASSERT(arg != static_cast<Thread::Argument>(NULL));

  // Transmission pump
  for (;;) {
    reinterpret_cast<DebugTransport *>(arg)->spin_tx();
  }
  return static_cast<Thread::Return>(0);
}


DebugTransport::DebugTransport(BaseChannel *channelp, char namebuf[])
:
  Transport(),
  rx_threadp(NULL),
  tx_threadp(NULL),
  channelp(channelp),
  namebufp(namebuf),
  subp_sem(false),
  send_lock(false),
  mgmt_rsub(*this, mgmt_msgqueue_buf, MGMT_BUFFER_LENGTH),
  mgmt_rpub()
{
  R2P_ASSERT(channelp != NULL);
  R2P_ASSERT(namebuf != NULL);
}


DebugTransport::~DebugTransport() {}


} // namespace r2p
