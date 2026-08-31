#ifndef EXN_INTERFACES_H
#define EXN_INTERFACES_H

/*
 * EXN packet/application interface constants.
 *
 * Wire profile:
 *  - CCSDS Space Packet version 0
 *  - CCSDSPack v2 PUS revision A for current EXN TC/TM services
 *  - TC source ID width: 1 octet
 *  - TM destination ID width: 0 octets
 *  - CRC-16/CCITT-FALSE packet error control
 *  - Application-data multi-byte integers: big-endian
 *
 * APID policy:
 *  - TC APID identifies the destination EXN endpoint.
 *  - TM APID identifies the producing EXN endpoint.
 */

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define EXN_PUS_TC_SOURCE_ID_OCTETS 1u
#define EXN_PUS_TM_DESTINATION_ID_OCTETS 0u
#define EXN_PUS_TC_ACK_FLAGS_DEFAULT 0u

/* Application Process Identifiers. */
typedef enum {
  APID_GS   = 0x0F0u,
  APID_MCU  = 0x100u,
  APID_PI   = 0x101u,
  APID_FPGA = 0x102u
} exn_apid_t;

/* PUS-A TC Source IDs. */
typedef enum {
  SRCID_MCU  = 0x01u,
  SRCID_PI   = 0x02u,
  SRCID_FPGA = 0x03u,
  SRCID_GS   = 0x10u
} exn_srcid_t;

/* EXN service identifiers. All currently use the PUS revision-A header profile. */
typedef enum {
  SVC_HK      = 3u,
  SVC_EVENTS  = 5u,
  SVC_TIME    = 17u,
  SVC_PARAM   = 20u,
  SVC_XFER    = 23u,
  SVC_CAM     = 200u,
  SVC_FPGA    = 210u,
  SVC_GS_LINK = 250u
} exn_service_t;

typedef enum {
  SUB_HK_REQ        = 1u,
  SUB_HK_REPORT     = 2u,
  SUB_SYS_HK_REQ    = 10u,
  SUB_SYS_HK_REPORT = 100u,

  SUB_EVENT_INFO  = 1u,
  SUB_EVENT_WARN  = 2u,
  SUB_EVENT_ERROR = 3u,

  SUB_TIME_SET    = 1u,
  SUB_TIME_REPORT = 2u,

  SUB_PARAM_SET = 1u,
  SUB_PARAM_GET = 2u,
  SUB_PARAM_VAL = 3u,

  SUB_XFER_START = 1u,
  SUB_XFER_STOP  = 2u,
  SUB_XFER_META  = 10u,
  SUB_XFER_CHUNK = 11u,
  SUB_XFER_DONE  = 12u,

  SUB_CAM_CAPTURE = 1u,
  SUB_CAM_SET     = 2u,
  SUB_CAM_GET     = 3u,
  SUB_CAM_REPORT  = 4u,
  SUB_CAM_ACK     = 5u,

  SUB_FPGA_EXEC   = 1u,
  SUB_FPGA_SET    = 2u,
  SUB_FPGA_GET    = 3u,
  SUB_FPGA_REPORT = 4u,
  SUB_FPGA_ACK    = 5u,

  SUB_GS_LINK_ACK = 1u
} exn_subservice_t;

typedef enum {
  RESULT_OK          = 0u,
  RESULT_INVALID     = 1u,
  RESULT_BUSY        = 2u,
  RESULT_UNSUPPORTED = 3u,
  RESULT_TIMEOUT     = 4u,
  RESULT_INTERNAL    = 5u
} exn_result_t;

typedef enum {
  TLV_U8    = 1u,
  TLV_U16   = 2u,
  TLV_U32   = 3u,
  TLV_I32   = 4u,
  TLV_F32   = 5u,
  TLV_STR   = 6u,
  TLV_BYTES = 7u,
  TLV_U64   = 8u,
  TLV_BOOL  = 9u
} exn_tlv_type_t;

typedef enum {
  PIX_RGB888      = 1u,
  PIX_GRAY8       = 2u,
  PIX_GRAY16      = 3u,
  PIX_YUV420      = 4u,
  PIX_BAYER_RGGB8 = 5u
} exn_pixel_t;

static inline void be_put_u16(uint8_t *p, uint16_t v) {
  p[0] = (uint8_t)(v >> 8);
  p[1] = (uint8_t)v;
}
static inline void be_put_u32(uint8_t *p, uint32_t v) {
  p[0] = (uint8_t)(v >> 24);
  p[1] = (uint8_t)(v >> 16);
  p[2] = (uint8_t)(v >> 8);
  p[3] = (uint8_t)v;
}
static inline uint16_t be_get_u16(const uint8_t *p) {
  return (uint16_t)((((uint16_t)p[0]) << 8) | p[1]);
}
static inline uint32_t be_get_u32(const uint8_t *p) {
  return (((uint32_t)p[0]) << 24) | (((uint32_t)p[1]) << 16) |
         (((uint32_t)p[2]) << 8) | p[3];
}

#if defined(__GNUC__)
#define EXN_PACKED __attribute__((packed))
#else
#define EXN_PACKED
#endif

/* GS -> MCU downstream-device proxy preamble. Not used for MCU-local 3/10 System HK. */
typedef struct EXN_PACKED {
  uint16_t transactionId;
  uint8_t target;  /* 1=PI, 2=FPGA */
  uint8_t options; /* bit0=mirror to GS, bit1=mirror to peer */
} exn_proxy_preamble_t;

typedef struct EXN_PACKED {
  uint64_t uptime_ms;
  int16_t temperature_cC;
  uint16_t status_flags;
  uint16_t last_error;
  uint8_t ts_cuc[6];
} exn_hk_generic_t;

typedef struct EXN_PACKED {
  uint16_t transactionId;
  uint8_t include_mask; /* bit0=MCU, bit1=PI, bit2=FPGA */
  uint16_t detailMask;
} exn_sys_hk_req_t;

typedef struct EXN_PACKED {
  uint16_t transactionId;
  uint8_t present_mask;
  uint8_t status;   /* 0=OK,1=PARTIAL,2=TIMEOUT,3=ERROR */
  uint8_t reserved; /* 0 */
  /* followed by repeated [block_len:u16][node_hk_bytes...] */
} exn_sys_hk_tm_hdr_t;

typedef struct EXN_PACKED {
  uint8_t mode;        /* 0=single, 1=burst */
  uint16_t burst_count;
  uint32_t exposure_us; /* 0=auto */
} exn_cam_capture_tc_t;

typedef struct EXN_PACKED {
  uint8_t orig_service;
  uint8_t orig_sub;
  uint8_t resultCode;
  uint16_t detail;
} exn_ack_tm_t;

typedef struct EXN_PACKED {
  uint16_t imageId;
  uint16_t height;
  uint16_t width;
  uint8_t channels;
  uint8_t pixel_type;
  uint32_t total_size;
  uint16_t chunk_size;
  uint8_t ts_cuc[6];
} exn_xfer_meta_tm_t;

typedef struct EXN_PACKED {
  uint16_t imageId;
  uint32_t offset;
  /* followed by chunk bytes */
} exn_xfer_chunk_tm_hdr_t;

typedef struct EXN_PACKED {
  uint16_t imageId;
  uint16_t totalChunks;
} exn_xfer_done_tm_t;

typedef struct EXN_PACKED {
  uint16_t transactionId;
  uint8_t ackCode; /* 0=Accepted,1=Rejected,2=Cancelled,3=Invalid,4=Timeout */
  uint16_t detail;
} exn_gs_link_ack_tm_t;

#ifdef __cplusplus
}
#endif

#endif /* EXN_INTERFACES_H */
