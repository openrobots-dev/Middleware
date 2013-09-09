#pragma once

#include <r2p/common.hpp>
#include <r2p/Message.hpp>
#include <r2p/NamingTraits.hpp>

namespace r2p {


struct IhexRecord {
  enum { MAX_DATA_LENGTH = 16 };

  enum TypeEnum {
    DATA = 0,
    END_OF_FILE,
    EXTENDED_SEGMENT_ADDRESS,
    START_SEGMENT_ADDRESS,
    EXTENDED_LINEAR_ADDRESS,
    START_LINEAR_ADDRESS,
  };

  uint8_t   count;
  uint16_t  offset;
  uint8_t   type;
  uint8_t   data[MAX_DATA_LENGTH];
  uint8_t   checksum;

  uint8_t compute_checksum() const;
} R2P_PACKED;


class BootMsg : public Message {
public:
  enum TypeEnum {
    NACK = 0,
    ACK,

    REMOVE_LAST,
    REMOVE_ALL,

    BEGIN_LOADER,
    END_LOADER,
    LINKING_SETUP,
    LINKING_ADDRESSES,
    LINKING_OUTCOME,
    IHEX_RECORD,

    BEGIN_APPINFO,
    END_APPINFO,
    APPINFO_SUMMARY,

    BEGIN_SETPARAM,
    END_SETPARAM,
    BEGIN_GETPARAM,
    END_GETPARAM,
    PARAM_REQUEST,
    PARAM_CHUNK
  };

  enum { MAX_PAYLOAD_LENGTH = 26 };

  typedef const uint8_t *Address;
  typedef size_t        Length;

  struct LinkingSetup {
    Length  pgmlen;
    Length  bsslen;
    Length  datalen;
    Length  stacklen;
    char    name[NamingTraits<Node>::MAX_LENGTH];
  } R2P_PACKED;

  struct LinkingAddresses {
    Address infoadr;
    Address pgmadr;
    Address bssadr;
    Address dataadr;
    Address datapgmadr;
    Address freeadr;
  } R2P_PACKED;

  struct LinkingOutcome {
    Address threadadr;
    Address cfgadr;
    Address appinfoadr;
  } R2P_PACKED;

  struct AppInfoSummary {
    Length  numapps;
    Address freeadr;
  } R2P_PACKED;

  struct ParamRequest {
    Length  offset;
    char    appname[NamingTraits<Node>::MAX_LENGTH];
    uint8_t length;
  } R2P_PACKED;

  struct ParamChunk {
    enum { MAX_DATA_LENGTH = 16 };

    uint8_t data[MAX_DATA_LENGTH];
  } R2P_PACKED;

public:
  union {
    uint8_t             raw[MAX_PAYLOAD_LENGTH];
    IhexRecord          ihex_record;
    LinkingSetup        linking_setup;
    LinkingAddresses    linking_addresses;
    LinkingOutcome      linking_outcome;
    AppInfoSummary      appinfo_summary;
    ParamRequest        param_request;
    ParamChunk          param_chunk;
  };
  uint8_t   type;
} R2P_PACKED;



} // namespace r2p
