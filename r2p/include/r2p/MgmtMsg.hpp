#pragma once

#include <r2p/common.hpp>
#include <r2p/Message.hpp>
#include <r2p/NamingTraits.hpp>

namespace r2p {

class Transport;


class MgmtMsg : public Message {
public:
  enum TypeEnum {
    RAW                         = 0x00,

    INFO_MODULE                 = 0x10,
    INFO_ADVERTISEMENT          = 0x11,
    INFO_SUBSCRIPTION           = 0x12,

    CMD_ADVERTISE               = 0x20,
    CMD_SUBSCRIBE_REQUEST       = 0x21,
    CMD_SUBSCRIBE_RESPONSE      = 0x22,

    CMD_STOP                    = 0x30,
    CMD_REBOOT                  = 0x31,
    CMD_BOOTLOAD                = 0x32,
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
        MAX_PAYLOAD_LENGTH - NamingTraits<Topic>::MAX_LENGTH -
        sizeof(Transport *) - sizeof(size_t)
    };

    char topic[NamingTraits<Topic>::MAX_LENGTH];
    uint8_t payload_size;
    uint8_t queue_length;
    uint8_t raw_params[MAX_RAW_PARAMS_LENGTH];
  } R2P_PACKED;

  struct Module {
    char name[NamingTraits<Middleware>::MAX_LENGTH];
    struct {
      unsigned stopped    : 1;
    } flags;
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
