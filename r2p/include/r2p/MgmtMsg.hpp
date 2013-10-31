#pragma once

#include <r2p/common.hpp>
#include <r2p/Message.hpp>
#include <r2p/NamingTraits.hpp>

namespace r2p {

class Transport;


class MgmtMsg : public Message {
public:
  enum TypeEnum {
    RAW                 = 0x00,

    // Module messages
    ALIVE               = 0x11,
    STOP                = 0x12,
    REBOOT              = 0x13,
    BOOTLOAD            = 0x14,

    // PubSub messages
    ADVERTISE           = 0x21,
    SUBSCRIBE_REQUEST   = 0x22,
    SUBSCRIBE_RESPONSE  = 0x23,

    // Path messages
    PATH                = 0x31,
  };

  enum { MAX_PAYLOAD_LENGTH = 31 };

  struct Path {
    char module[NamingTraits<Middleware>::MAX_LENGTH];
    char node[NamingTraits<Node>::MAX_LENGTH];
    char topic[NamingTraits<Topic>::MAX_LENGTH];
  } R2P_PACKED;

  struct PubSub {
    enum {
      MAX_RAW_PARAMS_LENGTH =
        MAX_PAYLOAD_LENGTH - NamingTraits<Topic>::MAX_LENGTH - sizeof(size_t)
    };

    char topic[NamingTraits<Topic>::MAX_LENGTH];
    uint8_t payload_size;
    uint8_t queue_length;
    uint8_t raw_params[MAX_RAW_PARAMS_LENGTH];
  } R2P_PACKED;

  struct Module {
    char name[NamingTraits<Middleware>::MAX_LENGTH];
    uint8_t reserved_;
    struct {
      unsigned  stopped     : 1;
      unsigned  rebooted    : 1;
      unsigned  boot_mode   : 1;
      unsigned  reserved_   : 5;
    } R2P_PACKED flags;
  } R2P_PACKED;

public:
  union {
    uint8_t payload[MAX_PAYLOAD_LENGTH];
    Path    path;
    PubSub  pubsub;
    Module  module;
  } R2P_PACKED;
  uint8_t type;

} R2P_PACKED;


}// namespace r2p
