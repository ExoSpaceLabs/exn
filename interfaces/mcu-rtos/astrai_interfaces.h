#ifndef ASTRAI_INTERFACES_H
#define ASTRAI_INTERFACES_H

/*
 * ASTRAi — MCU RTOS Header-only Interfaces
 *
 * Purpose
 *  - Provide compile-time definitions for CCSDS/PUS-based packet interfaces on STM32 MCU builds
 *    where a filesystem may not be available to load CCSDSPack .cfg files.
 *  - Mirrors the master ICD (docs/ICD.md) and device ICDs (docs/icd/*).
 *
 * Usage
 *  - Include this header in MCU projects to access APIDs, Source IDs, Service/Subservice enums,
 *    TLV type codes, result codes, and common application data payload structs.
 *  - Packets can then be constructed programmatically using CCSDSPack APIs or your own framing.
 *
 * Notes
 *  - Multi-byte fields in Application Data are big-endian (network order) per ICD.
 *  - Where structs are provided, they describe field layout but do not enforce endianness conversion;
 *    use helper functions/macros to write big-endian to the wire.
 */

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* APIDs (Application Process IDs) */
typedef enum {
  APID_GS   = 0x0F0u, /* Ground Station (PC simulator) */
  APID_MCU  = 0x100u, /* STM32 MCU-RTOS */
  APID_PI   = 0x101u, /* Raspberry Pi Camera Node */
  APID_FPGA = 0x102u  /* FPGA Processing Node */
} astr_ai_apid_t;

/* PUS Source IDs */
typedef enum {
  SRCID_MCU  = 0x01u,
  SRCID_PI   = 0x02u,
  SRCID_FPGA = 0x03u,
  SRCID_GS   = 0x10u
} astr_ai_srcid_t;

/* Services (PUS-A unless otherwise specified) */
typedef enum {
  SVC_HK      = 3u,   /* Housekeeping */
  SVC_EVENTS  = 5u,   /* PUS-B Event Reporting */
  SVC_TIME    = 17u,  /* PUS-C Time Management */
  SVC_PARAM   = 20u,  /* Parameter Management */
  SVC_XFER    = 23u,  /* Data Transfer */
  SVC_CAM     = 200u, /* Camera Control */
  SVC_FPGA    = 210u, /* FPGA Processing Control */
  SVC_GS_LINK = 250u  /* GS Link/Proxy ACK */
} astr_ai_service_t;

/* Subservices (selected) */
typedef enum {
  /* Service 3 */
  SUB_HK_REQ        = 1u,   /* TC 3/1 */
  SUB_HK_REPORT     = 2u,   /* TM 3/2 */
  SUB_SYS_HK_REQ    = 10u,  /* TC 3/10 */
  SUB_SYS_HK_REPORT = 100u, /* TM 3/100 */

  /* Service 17 */
  SUB_TIME_SET    = 1u, /* TC 17/1, PUS-C */
  SUB_TIME_REPORT = 2u, /* TM 17/2, PUS-C */

  /* Service 20 */
  SUB_PARAM_SET = 1u,
  SUB_PARAM_GET = 2u,
  SUB_PARAM_VAL = 3u,

  /* Service 23 */
  SUB_XFER_START = 1u,
  SUB_XFER_STOP  = 2u,
  SUB_XFER_META  = 10u,
  SUB_XFER_CHUNK = 11u,
  SUB_XFER_DONE  = 12u,

  /* Service 200 */
  SUB_CAM_CAPTURE = 1u,
  SUB_CAM_SET     = 2u,
  SUB_CAM_GET     = 3u,
  SUB_CAM_REPORT  = 4u,
  SUB_CAM_ACK     = 5u,

  /* Service 210 */
  SUB_FPGA_EXEC   = 1u,
  SUB_FPGA_SET    = 2u,
  SUB_FPGA_GET    = 3u,
  SUB_FPGA_REPORT = 4u,
  SUB_FPGA_ACK    = 5u,

  /* Service 250 */
  SUB_GS_LINK_ACK = 1u
} astr_ai_subservice_t;

/* Result / ACK codes (device ACKs e.g., CAM/FPGA) */
typedef enum {
  RESULT_OK          = 0u,
  RESULT_INVALID     = 1u,
  RESULT_BUSY        = 2u,
  RESULT_UNSUPPORTED = 3u,
  RESULT_TIMEOUT     = 4u,
  RESULT_INTERNAL    = 5u
} astr_ai_result_t;

/* TLV Type codes used in Application Data */
typedef enum {
  TLV_U8   = 1u,
  TLV_U16  = 2u,
  TLV_U32  = 3u,
  TLV_I32  = 4u,
  TLV_F32  = 5u,
  TLV_STR  = 6u,
  TLV_BYTES= 7u,
  TLV_U64  = 8u,
  TLV_BOOL = 9u
} astr_ai_tlv_type_t;

/* Pixel Types */
typedef enum {
  PIX_RGB888    = 1u,
  PIX_GRAY8     = 2u,
  PIX_GRAY16    = 3u,
  PIX_YUV420    = 4u,
  PIX_BAYER_RGGB8 = 5u
} astr_ai_pixel_t;

/* Helper: big-endian put/get for 16/32-bit (no unaligned access assumptions) */
static inline void be_put_u16(uint8_t *p, uint16_t v){ p[0]=(uint8_t)(v>>8); p[1]=(uint8_t)(v); }
static inline void be_put_u32(uint8_t *p, uint32_t v){ p[0]=(uint8_t)(v>>24); p[1]=(uint8_t)(v>>16); p[2]=(uint8_t)(v>>8); p[3]=(uint8_t)(v); }
static inline uint16_t be_get_u16(const uint8_t *p){ return (uint16_t)((((uint16_t)p[0])<<8)|p[1]); }
static inline uint32_t be_get_u32(const uint8_t *p){ return (((uint32_t)p[0])<<24)|(((uint32_t)p[1])<<16)|(((uint32_t)p[2])<<8)|p[3]; }

/* Common payload layouts (packed for documentation; prefer explicit put/get when serializing) */
#if defined(__GNUC__)
#define ASTRAI_PACKED __attribute__((packed))
#else
#define ASTRAI_PACKED
#endif

/* HK Report (Service 3/2) — generic fields per ICD Section 4.1 */
typedef struct ASTRAI_PACKED {
  uint64_t uptime_ms;   /* big-endian on wire */
  int16_t  temperature_cC;
  uint16_t status_flags;
  uint16_t last_error;
  uint8_t  ts_cuc[6];
} astr_ai_hk_generic_t;

/* System HK Request (3/10) */
typedef struct ASTRAI_PACKED {
  uint16_t transactionId; /* 0 if unused */
  uint8_t  include_mask;  /* bit0=MCU, bit1=PI, bit2=FPGA */
  uint16_t detailMask;
} astr_ai_sys_hk_req_t;

/* System HK Report (3/100) header */
typedef struct ASTRAI_PACKED {
  uint16_t transactionId;
  uint8_t  present_mask; /* bit0=MCU, bit1=PI, bit2=FPGA */
  uint8_t  status;       /* 0=OK,1=PARTIAL,2=TIMEOUT,3=ERROR */
  uint8_t  reserved;     /* 0 */
  /* followed by node blocks: [block_len:u16][node_hk_bytes...] repeated */
} astr_ai_sys_hk_tm_hdr_t;

/* Camera Capture TC (200/1) */
typedef struct ASTRAI_PACKED {
  uint8_t  mode;        /* 0=single,1=burst */
  uint16_t burst_count; /* ignored if mode=single */
  uint32_t exposure_us; /* 0=auto */
} astr_ai_cam_capture_tc_t;

/* Camera ACK/NACK TM (200/5) and FPGA ACK/NACK TM (210/5) */
typedef struct ASTRAI_PACKED {
  uint8_t  orig_service; /* e.g., 200 */
  uint8_t  orig_sub;     /* e.g., 1 */
  uint8_t  resultCode;   /* see astr_ai_result_t */
  uint16_t detail;       /* optional */
} astr_ai_ack_tm_t;

/* Transfer Meta TM (23/10) */
typedef struct ASTRAI_PACKED {
  uint16_t imageId;
  uint16_t height;
  uint16_t width;
  uint8_t  channels;
  uint8_t  pixel_type; /* astr_ai_pixel_t */
  uint32_t total_size;
  uint16_t chunk_size;
  uint8_t  ts_cuc[6];
} astr_ai_xfer_meta_tm_t;

/* Transfer Chunk TM (23/11) header before data[] */
typedef struct ASTRAI_PACKED {
  uint16_t imageId;
  uint32_t offset;
  /* followed by data[<=chunk_size] */
} astr_ai_xfer_chunk_tm_hdr_t;

/* Transfer Complete TM (23/12) */
typedef struct ASTRAI_PACKED {
  uint16_t imageId;
  uint16_t totalChunks;
} astr_ai_xfer_done_tm_t;

/* GS Link/Proxy ACK (250/1) */
typedef struct ASTRAI_PACKED {
  uint16_t transactionId; /* 0 if N/A */
  uint8_t  ackCode;       /* 0=Accepted,1=Rejected,2=Cancelled,3=Invalid,4=Timeout */
  uint16_t detail;
} astr_ai_gs_link_ack_tm_t;

/* Proxy preamble used on GS→MCU device-directed TCs */
typedef struct ASTRAI_PACKED {
  uint16_t transactionId; /* 1..65535; 0 reserved */
  uint8_t  target;        /* 1=PI,2=FPGA */
  uint8_t  options;       /* bit0=mirror to GS, bit1=mirror to peer */
} astr_ai_proxy_preamble_t;

#ifdef __cplusplus
}
#endif

#endif /* ASTRAI_INTERFACES_H */
