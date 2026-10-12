#ifndef EXN_INTERFACES_H
#define EXN_INTERFACES_H

/*
 * EXN mission interface constants.
 * Normative source: ExoSpaceLabs/exn docs/ICD.md, docs/services.md,
 * docs/component-services.md, and docs/icd/.
 *
 * Active profile:
 *  - CCSDS Space Packet version 0
 *  - ECSS PUS Revision C
 *  - PUS-C source/destination identifiers: 16 bits
 *  - CRC-16/CCITT-FALSE
 *  - big-endian mission Application Data
 */

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define EXN_PUS_TC_ACK_ACCEPTANCE        0x8u
#define EXN_PUS_TC_ACK_COMPLETION        0x1u
#define EXN_PUS_TC_ACK_START             0x4u
#define EXN_PUS_TC_ACK_PROGRESS          0x2u
#define EXN_PUS_TC_ACK_READ_DEFAULT      0x8u
#define EXN_PUS_TC_ACK_CHANGE_DEFAULT    0x9u
#define EXN_PUS_TC_ACK_LONG_DEFAULT      0xDu

typedef enum {
  APID_GS       = 0x0F0u,
  APID_OBC      = 0x100u,
  APID_PI_CAM   = 0x101u,
  APID_FPGA     = 0x102u,
  APID_ANTENNA  = 0x103u,
  APID_PI_MGMT  = 0x104u
} exn_apid_t;

typedef enum {
  PUSID_OBC      = 0x0001u,
  PUSID_PI_CAM   = 0x0002u,
  PUSID_FPGA     = 0x0003u,
  PUSID_ANTENNA  = 0x0004u,
  PUSID_PI_MGMT  = 0x0005u,
  PUSID_GS       = 0x0010u
} exn_pus_id_t;

typedef enum {
  SVC_VERIFY   = 1u,
  SVC_HK       = 3u,
  SVC_EVENTS   = 5u,
  SVC_FUNCTION = 8u,
  SVC_TIME     = 9u,
  SVC_MONITOR  = 12u,
  SVC_PARAM    = 20u,
  SVC_CAMERA   = 200u,
  SVC_PROCESS  = 210u,
  SVC_OBJECT   = 220u,
  SVC_SOFTWARE = 230u
} exn_service_t;

typedef enum {
  SUB_VERIFY_ACCEPT_OK       = 1u,
  SUB_VERIFY_ACCEPT_FAIL     = 2u,
  SUB_VERIFY_START_OK        = 3u,
  SUB_VERIFY_START_FAIL      = 4u,
  SUB_VERIFY_PROGRESS_OK     = 5u,
  SUB_VERIFY_PROGRESS_FAIL   = 6u,
  SUB_VERIFY_COMPLETE_OK     = 7u,
  SUB_VERIFY_COMPLETE_FAIL   = 8u,

  SUB_HK_ENABLE_PERIODIC     = 5u,
  SUB_HK_DISABLE_PERIODIC    = 6u,
  SUB_HK_REPORT              = 25u,
  SUB_HK_REQUEST             = 27u,
  SUB_SYS_HK_REQUEST         = 128u,
  SUB_SYS_HK_REPORT          = 129u,

  SUB_EVENT_INFO             = 1u,
  SUB_EVENT_LOW              = 2u,
  SUB_EVENT_MEDIUM           = 3u,
  SUB_EVENT_HIGH             = 4u,

  SUB_FUNCTION_PERFORM       = 1u,

  SUB_TIME_SET_MISSION       = 128u,

  SUB_MONITOR_ENABLE_DEF     = 1u,
  SUB_MONITOR_DISABLE_DEF    = 2u,
  SUB_MONITOR_GET_OOL        = 10u,
  SUB_MONITOR_OOL_REPORT     = 11u,
  SUB_MONITOR_GET_STATUS     = 13u,
  SUB_MONITOR_STATUS_REPORT  = 14u,
  SUB_MONITOR_ENABLE         = 15u,
  SUB_MONITOR_DISABLE        = 16u,

  SUB_PARAM_REPORT_REQUEST   = 1u,
  SUB_PARAM_REPORT           = 2u,
  SUB_PARAM_SET              = 3u,

  SUB_CAM_START              = 1u,
  SUB_CAM_STOP               = 2u,
  SUB_CAM_SET_CONTROLS       = 3u,
  SUB_CAM_GET_CONTROLS       = 4u,
  SUB_CAM_CONTROLS_REPORT    = 5u,
  SUB_CAM_GET_CAPABILITIES   = 6u,
  SUB_CAM_CAPABILITIES       = 7u,
  SUB_CAM_LIST_PRODUCTS      = 8u,
  SUB_CAM_PRODUCT_CATALOG    = 9u,
  SUB_CAM_DELETE_PRODUCT     = 10u,
  SUB_CAM_GET_ACQ_STATUS     = 11u,
  SUB_CAM_ACQ_STATUS         = 12u,

  SUB_PROCESS_SUBMIT         = 1u,
  SUB_PROCESS_CANCEL         = 2u,
  SUB_PROCESS_GET_STATUS     = 3u,
  SUB_PROCESS_STATUS         = 4u,
  SUB_PROCESS_GET_CAPS       = 5u,
  SUB_PROCESS_CAPS           = 6u,
  SUB_PROCESS_SMALL_RESULT   = 7u,

  SUB_OBJECT_START           = 1u,
  SUB_OBJECT_STOP            = 2u,
  SUB_OBJECT_GET_STATUS      = 3u,
  SUB_OBJECT_METADATA        = 10u,
  SUB_OBJECT_CHUNK           = 11u,
  SUB_OBJECT_COMPLETE        = 12u,
  SUB_OBJECT_STATUS          = 13u,

  SUB_SW_GET_INVENTORY       = 1u,
  SUB_SW_INVENTORY           = 2u,
  SUB_SW_STAGE               = 3u,
  SUB_SW_ACTIVATE            = 4u,
  SUB_SW_ROLLBACK            = 5u,
  SUB_SW_GET_STATUS          = 6u,
  SUB_SW_STATUS              = 7u
} exn_subservice_t;

typedef enum {
  EXN_FUNCTION_CLEAR_FAULTS          = 0x0001u,
  EXN_FUNCTION_REINITIALIZE          = 0x0002u,
  EXN_FUNCTION_REBOOT_NODE           = 0x0003u,
  EXN_FUNCTION_ENTER_IDLE            = 0x0004u,
  EXN_FUNCTION_LEAVE_IDLE            = 0x0005u,
  EXN_FUNCTION_CREATE_DIAGNOSTIC_LOG = 0x0006u,

  EXN_FUNCTION_PI_RESTART_CAMERA     = 0x0101u,
  EXN_FUNCTION_PI_RESTART_ANTENNA    = 0x0102u,
  EXN_FUNCTION_PI_RESTART_MISSION    = 0x0103u,
  EXN_FUNCTION_PI_REBOOT             = 0x0104u,
  EXN_FUNCTION_PI_SHUTDOWN           = 0x0105u
} exn_function_id_t;

typedef enum {
  EXN_VALUE_BOOL   = 1u,
  EXN_VALUE_U8     = 2u,
  EXN_VALUE_U16    = 3u,
  EXN_VALUE_U32    = 4u,
  EXN_VALUE_U64    = 5u,
  EXN_VALUE_I16    = 6u,
  EXN_VALUE_I32    = 7u,
  EXN_VALUE_F32    = 8u,
  EXN_VALUE_STRING = 9u,
  EXN_VALUE_BYTES  = 10u
} exn_value_type_t;

typedef enum {
  EXN_TARGET_PI_CAM  = 1u,
  EXN_TARGET_FPGA    = 2u,
  EXN_TARGET_ANTENNA = 3u,
  EXN_TARGET_PI_MGMT = 4u
} exn_route_target_t;

typedef enum {
  EXN_HK_HEARTBEAT = 1u,
  EXN_HK_HEALTH    = 2u,
  EXN_HK_SENSORS   = 3u,
  EXN_HK_LINKS     = 4u
} exn_hk_structure_t;

typedef enum {
  EXN_HW_STM32H755_NUCLEO = 0x0001u,
  EXN_HW_RPI5             = 0x0101u,
  EXN_HW_KRIA_K26         = 0x0201u,
  EXN_HW_KRIA_KV260       = 0x0202u
} exn_hardware_profile_t;

typedef enum {
  EXN_SENSOR_OBC_MCU_JUNCTION_TEMP = 0x0101u,
  EXN_SENSOR_OBC_VDDA_ESTIMATE     = 0x0102u,
  EXN_SENSOR_OBC_VBAT              = 0x0103u,

  EXN_SENSOR_PI5_SOC_TEMP          = 0x1101u,
  EXN_SENSOR_PI5_EXT5V             = 0x1102u,
  EXN_SENSOR_PI5_CORE_VOLTAGE      = 0x1103u,

  EXN_SENSOR_K26_JUNCTION_TEMP     = 0x2101u,
  EXN_SENSOR_K26_PS_0V85           = 0x2110u,
  EXN_SENSOR_K26_PS_1V80           = 0x2111u,
  EXN_SENSOR_K26_PL_0V72           = 0x2120u,
  EXN_SENSOR_K26_PL_1V80           = 0x2121u,
  EXN_SENSOR_K26_PL_0V85           = 0x2122u,

  EXN_SENSOR_KV260_SOM_5V_VOLTAGE = 0x2180u,
  EXN_SENSOR_KV260_SOM_5V_CURRENT = 0x2181u,
  EXN_SENSOR_KV260_SOM_5V_POWER   = 0x2182u
} exn_sensor_id_t;

typedef enum {
  EXN_SENSOR_VALID       = 0u,
  EXN_SENSOR_UNAVAILABLE = 1u,
  EXN_SENSOR_STALE       = 2u,
  EXN_SENSOR_FAULT       = 3u
} exn_sensor_status_t;

typedef enum {
  EXN_ALARM_NORMAL    = 0u,
  EXN_ALARM_WARN_LOW  = 1u,
  EXN_ALARM_WARN_HIGH = 2u,
  EXN_ALARM_CRIT_LOW  = 3u,
  EXN_ALARM_CRIT_HIGH = 4u
} exn_sensor_alarm_t;

#define EXN_PI_HEALTH_UNDERVOLTAGE_NOW       (1u << 0)
#define EXN_PI_HEALTH_FREQ_CAPPED_NOW        (1u << 1)
#define EXN_PI_HEALTH_THROTTLED_NOW          (1u << 2)
#define EXN_PI_HEALTH_SOFT_TEMP_LIMIT_NOW    (1u << 3)
#define EXN_PI_HEALTH_UNDERVOLTAGE_OCCURRED  (1u << 16)
#define EXN_PI_HEALTH_FREQ_CAPPED_OCCURRED   (1u << 17)
#define EXN_PI_HEALTH_THROTTLED_OCCURRED     (1u << 18)
#define EXN_PI_HEALTH_SOFT_TEMP_OCCURRED     (1u << 19)

typedef enum {
  EXN_STATE_UNKNOWN  = 0u,
  EXN_STATE_STARTING = 1u,
  EXN_STATE_READY    = 2u,
  EXN_STATE_BUSY     = 3u,
  EXN_STATE_DEGRADED = 4u,
  EXN_STATE_ERROR    = 5u,
  EXN_STATE_UPDATING = 6u,
  EXN_STATE_OFFLINE  = 7u
} exn_endpoint_state_t;

typedef enum {
  EXN_SUPERVISION_UNKNOWN = 0u,
  EXN_SUPERVISION_OK      = 1u,
  EXN_SUPERVISION_STALE   = 2u,
  EXN_SUPERVISION_OFFLINE = 3u,
  EXN_SUPERVISION_ERROR   = 4u
} exn_supervision_state_t;

typedef enum {
  EXN_RECOVERY_NONE               = 0u,
  EXN_RECOVERY_PENDING            = 1u,
  EXN_RECOVERY_ACTION_SENT        = 2u,
  EXN_RECOVERY_WAITING_FOR_HEALTH = 3u,
  EXN_RECOVERY_RECOVERED          = 4u,
  EXN_RECOVERY_FAILED             = 5u,
  EXN_RECOVERY_SUPPRESSED         = 6u
} exn_recovery_state_t;

typedef enum {
  EXN_RECOVERY_REASON_NONE              = 0u,
  EXN_RECOVERY_REASON_HEARTBEAT_TIMEOUT = 1u,
  EXN_RECOVERY_REASON_HEALTH_TIMEOUT    = 2u,
  EXN_RECOVERY_REASON_PROTOCOL_ERROR    = 3u,
  EXN_RECOVERY_REASON_CRASH_LOOP        = 4u,
  EXN_RECOVERY_REASON_THERMAL           = 5u,
  EXN_RECOVERY_REASON_POWER_FAULT       = 6u,
  EXN_RECOVERY_REASON_MANUAL            = 7u
} exn_recovery_reason_t;

typedef enum {
  EXN_RECOVERY_ACTION_NONE            = 0u,
  EXN_RECOVERY_ACTION_REINITIALIZE    = 1u,
  EXN_RECOVERY_ACTION_RESTART_SERVICE = 2u,
  EXN_RECOVERY_ACTION_SOFT_REBOOT     = 3u,
  EXN_RECOVERY_ACTION_HARD_RESET      = 4u,
  EXN_RECOVERY_ACTION_POWER_CYCLE     = 5u
} exn_recovery_action_t;

typedef enum {
  EXN_RECOVERY_RESULT_NONE       = 0u,
  EXN_RECOVERY_RESULT_SUCCESS    = 1u,
  EXN_RECOVERY_RESULT_FAILED     = 2u,
  EXN_RECOVERY_RESULT_SUPPRESSED = 3u
} exn_recovery_result_t;

#define EXN_RECOVERY_FLAG_AUTO_ENABLED         (1u << 0)
#define EXN_RECOVERY_FLAG_COOLDOWN_ACTIVE      (1u << 1)
#define EXN_RECOVERY_FLAG_RETRY_LIMIT_REACHED  (1u << 2)
#define EXN_RECOVERY_FLAG_HARD_RESET_AVAILABLE (1u << 3)
#define EXN_RECOVERY_FLAG_POWER_CYCLE_AVAILABLE (1u << 4)

typedef enum {
  EXN_SYSTEM_HK_OK       = 0u,
  EXN_SYSTEM_HK_DEGRADED = 1u,
  EXN_SYSTEM_HK_ERROR    = 2u
} exn_system_hk_status_t;

typedef enum {
  EXN_TIME_UNSYNCED = 0u,
  EXN_TIME_SYNCED   = 1u,
  EXN_TIME_HOLDOVER = 2u,
  EXN_TIME_INVALID  = 3u
} exn_time_state_t;

typedef enum {
  EXN_OBJECT_CAPTURED_IMAGE  = 0x0001u,
  EXN_OBJECT_DERIVED_IMAGE   = 0x0002u,
  EXN_OBJECT_PROCESSING_MASK = 0x0003u,
  EXN_OBJECT_RESULT_BLOB     = 0x0004u,
  EXN_OBJECT_THUMBNAIL       = 0x0005u,
  EXN_OBJECT_UPDATE_PACKAGE  = 0x0010u,
  EXN_OBJECT_DIAGNOSTIC_LOG  = 0x0020u,
  EXN_OBJECT_OPAQUE          = 0x7FFFu
} exn_object_type_t;

typedef enum {
  EXN_FORMAT_SENSOR_RAW = 0x0001u,
  EXN_FORMAT_DNG        = 0x0002u,
  EXN_FORMAT_TIFF       = 0x0003u,
  EXN_FORMAT_JPEG       = 0x0004u,
  EXN_FORMAT_PNG        = 0x0005u,
  EXN_FORMAT_BINARY     = 0x0006u,
  EXN_FORMAT_CBOR       = 0x0007u,
  EXN_FORMAT_JSON       = 0x0008u
} exn_object_format_t;

static inline void exn_be_put_u16(uint8_t *p, uint16_t v) {
  p[0] = (uint8_t)(v >> 8); p[1] = (uint8_t)v;
}
static inline void exn_be_put_u32(uint8_t *p, uint32_t v) {
  p[0] = (uint8_t)(v >> 24); p[1] = (uint8_t)(v >> 16);
  p[2] = (uint8_t)(v >> 8); p[3] = (uint8_t)v;
}
static inline void exn_be_put_u64(uint8_t *p, uint64_t v) {
  exn_be_put_u32(p, (uint32_t)(v >> 32));
  exn_be_put_u32(p + 4, (uint32_t)v);
}
static inline uint16_t exn_be_get_u16(const uint8_t *p) {
  return (uint16_t)((((uint16_t)p[0]) << 8) | p[1]);
}
static inline uint32_t exn_be_get_u32(const uint8_t *p) {
  return (((uint32_t)p[0]) << 24) | (((uint32_t)p[1]) << 16)
       | (((uint32_t)p[2]) << 8) | p[3];
}
static inline uint64_t exn_be_get_u64(const uint8_t *p) {
  return (((uint64_t)exn_be_get_u32(p)) << 32) | exn_be_get_u32(p + 4);
}

/* Fixed prefixes only. Variable-length bodies are encoded explicitly. */
#if defined(__GNUC__)
#define EXN_PACKED __attribute__((packed))
#else
#define EXN_PACKED
#endif

typedef struct EXN_PACKED {
  uint8_t packet_id_be[2];
  uint8_t packet_sequence_control_be[2];
} exn_request_id_t;

typedef struct EXN_PACKED {
  exn_request_id_t request_id;
  uint8_t failure_code_be[2];
  uint8_t failure_data_length_be[2];
} exn_verification_failure_prefix_t;

typedef struct EXN_PACKED {
  uint8_t transaction_id_be[2];
  uint8_t target;
  uint8_t options;
} exn_route_envelope_t;

/* TC[3,128] System HK Request has no Application Data. */

typedef struct EXN_PACKED {
  exn_request_id_t request_id;
  uint8_t snapshot_unix_ms_be[8];
  uint8_t system_status;
  uint8_t endpoint_count;
} exn_system_hk_report_prefix_t;

typedef struct EXN_PACKED {
  uint8_t producer_apid_be[2];
  uint8_t supervision_state;
  uint8_t endpoint_state;
  uint8_t recovery_state;
  uint8_t recovery_reason;
  uint8_t recovery_action;
  uint8_t recovery_flags;
  uint8_t recovery_attempts_be[2];
  uint8_t recovery_successes_be[2];
  uint8_t last_recovery_result;
  uint8_t heartbeat_age_ms_be[4];
  uint8_t health_age_ms_be[4];
  uint8_t last_recovery_unix_ms_be[8];
  uint8_t structure_count;
} exn_system_hk_endpoint_prefix_t;

typedef struct EXN_PACKED {
  uint8_t structure_id_be[2];
  uint8_t age_ms_be[4];
  uint8_t block_length_be[2];
} exn_system_hk_structure_prefix_t;

typedef struct EXN_PACKED {
  uint8_t structure_id_be[2];
  uint8_t sample_unix_ms_be[8];
} exn_hk_report_prefix_t;

typedef struct EXN_PACKED {
  uint8_t hardware_profile_id_be[2];
  uint8_t count_be[2];
} exn_sensors_report_prefix_t;

typedef struct EXN_PACKED {
  uint8_t unix_ms_be[8];
  uint8_t source;
  uint8_t uncertainty_ms_be[2];
  uint8_t sequence_be[2];
} exn_set_mission_time_t;

typedef struct EXN_PACKED {
  uint8_t acquisition_id_be[4];
  uint8_t mode;
  uint8_t count_be[2];
  uint8_t interval_ms_be[4];
  uint8_t duration_ms_be[4];
  uint8_t product_mask_be[2];
  uint8_t flags_be[2];
} exn_camera_start_t;

typedef struct EXN_PACKED {
  uint8_t transfer_id_be[4];
  uint8_t object_id_be[8];
  uint8_t destination_apid_be[2];
  uint8_t chunk_size_be[2];
  uint8_t flags_be[2];
} exn_object_transfer_start_t;

typedef struct EXN_PACKED {
  uint8_t transfer_id_be[4];
  uint8_t object_id_be[8];
  uint8_t offset_be[8];
  uint8_t data_length_be[2];
} exn_object_chunk_prefix_t;

typedef struct EXN_PACKED {
  uint8_t job_id_be[4];
  uint8_t operation_id_be[2];
  uint8_t input_object_id_be[8];
  uint8_t parameter_set_id_be[2];
  uint8_t destination_count;
  /* followed by destination_apid[destination_count]:u16 and flags:u16 */
} exn_processing_job_prefix_t;

#ifdef __cplusplus
}
#endif

#endif /* EXN_INTERFACES_H */
