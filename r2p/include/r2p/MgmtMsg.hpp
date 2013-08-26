#pragma once

#include <r2p/common.hpp>
#include <r2p/Message.hpp>
#include <r2p/NamingTraits.hpp>

namespace r2p {

class Transport;


class MgmtMsg : public Message {
public:
  enum TypeEnum {
    RAW                     = 0x00,

    INFO_MODULE             = 0x10,
    INFO_ADVERTISEMENT      = 0x11,
    INFO_SUBSCRIPTION       = 0x12,

    CMD_GET_NETWORK_STATE   = 0x20,
    CMD_ADVERTISE           = 0x21,
    CMD_SUBSCRIBE           = 0x22,
    CMD_BOOTLOADER          = 0x23,
  };

public:
  uint8_t type;

  union {
    uint8_t payload[31];

    struct {
      char module[NamingTraits<Middleware>::MAX_LENGTH];
      char node[NamingTraits<Node>::MAX_LENGTH];
      char topic[NamingTraits<Topic>::MAX_LENGTH];
    } path;

    struct {
      char topic[NamingTraits<Topic>::MAX_LENGTH];
      Transport *transportp;
      size_t queue_length;
      uint16_t rtcan_id; // FIXME: pubsub content should be specified by the transport to insert transport-dependent fields
    } pubsub;

    struct {
      char name[NamingTraits<Middleware>::MAX_LENGTH];
      struct {
        unsigned stopped    : 1;
      } flags;
    } module;

  } R2P_PACKED;

} R2P_PACKED;


}// namespace r2p
