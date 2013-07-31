#pragma once

#include <r2p/common.hpp>
#include <r2p/Message.hpp>
#include <r2p/NamingTraits.hpp>

namespace r2p {


class InfoMsg : public Message {
public:
  enum TypeEnum {
    RAW                 = 0x00,

    ADVERTISEMENT       = 0x10,
    SUBSCRIPTION        = 0x11,

    GET_NETWORK_STATE   = 0x20,
  };

public:
  uint8_t type;
  union {
    struct {
      char module[NamingTraits<Middleware>::MAX_LENGTH];
      char node[NamingTraits<Node>::MAX_LENGTH];
      char topic[NamingTraits<Topic>::MAX_LENGTH];
    } path;
    uint8_t payload[31];
  } R2P_PACKED;

} R2P_PACKED;


}// namespace r2p
